#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Input admission tests for the public WAV and MP3 conversion boundary."""

from __future__ import annotations

import subprocess
import unittest
from pathlib import Path
from unittest.mock import patch

from audio2haptic.core.preprocess import InputError
from audio2haptic.core.preprocess import MAX_DECODED_PCM_BYTES
from audio2haptic.core.preprocess import MAX_INPUT_DURATION_MS
from audio2haptic.core.preprocess import MAX_PROBE_OUTPUT_BYTES
from audio2haptic.core.preprocess import decode_to_mono_samples
from audio2haptic.core.preprocess import ensure_supported_suffix
from audio2haptic.core.preprocess import _ffmpeg_command
from audio2haptic.core.preprocess import _ffprobe_command
from audio2haptic.core.preprocess import load_audio_input
from audio2haptic.core.preprocess import probe_audio


def input_error_code(operation) -> str:
    try:
        operation()
    except InputError as exc:
        return exc.code
    raise AssertionError("Expected input processing to fail.")


def probe_result(payload: str, returncode: int = 0) -> subprocess.CompletedProcess:
    return subprocess.CompletedProcess(["ffprobe"], returncode, stdout=payload)


def decode_result(data: bytes, returncode: int = 0) -> subprocess.CompletedProcess:
    return subprocess.CompletedProcess(["ffmpeg"], returncode, stdout=data)


def oversized_probe_result(*_, **kwargs) -> subprocess.CompletedProcess:
    kwargs["stdout"].write("x" * (MAX_PROBE_OUTPUT_BYTES + 1))
    return subprocess.CompletedProcess(["ffprobe"], 0)


def stored_probe_result(*_, **kwargs) -> subprocess.CompletedProcess:
    kwargs["stdout"].write('{"streams": [{"channels": 2}], "format": {"duration": "1"}}')
    return subprocess.CompletedProcess(["ffprobe"], 0)


class PreprocessTests(unittest.TestCase):
    def test_suffix_accepts_only_wav_or_mp3(self) -> None:
        self.assertEqual(ensure_supported_suffix(Path("audio.WAV")), "wav")
        self.assertEqual(ensure_supported_suffix(Path("audio.mp3")), "mp3")
        self.assertEqual(
            input_error_code(lambda: ensure_supported_suffix(Path("audio.flac"))),
            "unsupported_format",
        )

    def test_probe_maps_missing_tool_process_failures_and_metadata_errors(self) -> None:
        with patch("audio2haptic.core.preprocess.shutil.which", return_value=None):
            self.assertEqual(input_error_code(lambda: probe_audio(Path("audio.wav"))), "ffprobe_missing")

        cases = (
            (probe_result("", returncode=1), "probe_failed"),
            (probe_result("{"), "probe_failed"),
            (probe_result("[]"), "probe_failed"),
            (probe_result('{"streams": {}, "format": {}}'), "probe_failed"),
            (probe_result('{"streams": [{"channels": "bad"}], "format": {"duration": "1"}}'), "probe_failed"),
            (probe_result('{"streams": [{"channels": 1}], "format": {"duration": "nan"}}'), "invalid_duration"),
            (probe_result('{"streams": [{"channels": 0}], "format": {"duration": "1"}}'), "invalid_channels"),
        )
        for result, expected in cases:
            with self.subTest(expected=expected):
                with (
                    patch("audio2haptic.core.preprocess.shutil.which", return_value="ffprobe"),
                    patch("audio2haptic.core.preprocess.subprocess.run", return_value=result),
                ):
                    self.assertEqual(input_error_code(lambda: probe_audio(Path("audio.wav"))), expected)

    def test_probe_handles_timeout_os_error_and_builds_local_only_command(self) -> None:
        for error, expected in (
            (subprocess.TimeoutExpired(["ffprobe"], timeout=1), "probe_timed_out"),
            (OSError("missing"), "probe_failed"),
        ):
            with self.subTest(expected=expected):
                with (
                    patch("audio2haptic.core.preprocess.shutil.which", return_value="ffprobe"),
                    patch("audio2haptic.core.preprocess.subprocess.run", side_effect=error),
                ):
                    self.assertEqual(input_error_code(lambda: probe_audio(Path("audio.wav"))), expected)

        valid = probe_result('{"streams": [{"channels": 2}], "format": {"duration": "1"}}')
        with (
            patch("audio2haptic.core.preprocess.shutil.which", return_value="ffprobe"),
            patch("audio2haptic.core.preprocess.subprocess.run", return_value=valid) as run,
        ):
            self.assertEqual(probe_audio(Path("audio.wav")), (2, 1_000))
        command = run.call_args.args[0]
        self.assertIn("-protocol_whitelist", command)
        self.assertIn("file", command)
        self.assertIn("-select_streams", command)
        self.assertIn("a:0", command)
        self.assertEqual(command[-1], f"file:{Path('audio.wav').resolve()}")
        self.assertEqual(run.call_args.kwargs["stdin"], subprocess.DEVNULL)

    def test_commands_use_a_file_uri_for_protocol_like_file_names(self) -> None:
        path = Path("file:untrusted.wav")
        expected = f"file:{path.resolve()}"

        self.assertEqual(_ffprobe_command("ffprobe", path)[-1], expected)
        command = _ffmpeg_command("ffmpeg", path)
        self.assertEqual(command[command.index("-i") + 1], expected)

    def test_probe_rejects_oversized_metadata_without_capturing_it_in_memory(self) -> None:
        with (
            patch("audio2haptic.core.preprocess.shutil.which", return_value="ffprobe"),
            patch("audio2haptic.core.preprocess.subprocess.run", side_effect=oversized_probe_result) as run,
        ):
            self.assertEqual(input_error_code(lambda: probe_audio(Path("audio.wav"))), "probe_output_too_large")
        self.assertIsNot(run.call_args.kwargs["stdout"], subprocess.PIPE)
        self.assertTrue(hasattr(run.call_args.kwargs["stdout"], "fileno"))

    def test_probe_reads_normal_metadata_from_the_temporary_output_file(self) -> None:
        with (
            patch("audio2haptic.core.preprocess.shutil.which", return_value="ffprobe"),
            patch("audio2haptic.core.preprocess.subprocess.run", side_effect=stored_probe_result),
        ):
            self.assertEqual(probe_audio(Path("audio.wav")), (2, 1_000))

    def test_decoder_maps_missing_tool_process_failures_and_output_bounds(self) -> None:
        with patch("audio2haptic.core.preprocess.shutil.which", return_value=None):
            self.assertEqual(input_error_code(lambda: decode_to_mono_samples(Path("audio.wav"))), "ffmpeg_missing")

        cases = (
            (decode_result(b"", returncode=1), "decode_failed"),
            (decode_result(b""), "empty_audio"),
            (decode_result(b"\0" * (MAX_DECODED_PCM_BYTES + 1)), "input_too_long"),
        )
        for result, expected in cases:
            with self.subTest(expected=expected):
                with (
                    patch("audio2haptic.core.preprocess.shutil.which", return_value="ffmpeg"),
                    patch("audio2haptic.core.preprocess.subprocess.run", return_value=result),
                ):
                    self.assertEqual(input_error_code(lambda: decode_to_mono_samples(Path("audio.wav"))), expected)

    def test_decoder_handles_timeout_os_error_and_valid_mono_output(self) -> None:
        for error, expected in (
            (subprocess.TimeoutExpired(["ffmpeg"], timeout=1), "decode_timed_out"),
            (OSError("missing"), "decode_failed"),
        ):
            with self.subTest(expected=expected):
                with (
                    patch("audio2haptic.core.preprocess.shutil.which", return_value="ffmpeg"),
                    patch("audio2haptic.core.preprocess.subprocess.run", side_effect=error),
                ):
                    self.assertEqual(input_error_code(lambda: decode_to_mono_samples(Path("audio.wav"))), expected)

        with (
            patch("audio2haptic.core.preprocess.shutil.which", return_value="ffmpeg"),
            patch("audio2haptic.core.preprocess.subprocess.run", return_value=decode_result(b"\0\0")) as run,
        ):
            samples, sample_rate, backend = decode_to_mono_samples(Path("audio.wav"))
        self.assertEqual((samples, sample_rate, backend), ([0.0], 16_000, "ffmpeg"))
        self.assertIn("-nostdin", run.call_args.args[0])
        self.assertEqual(run.call_args.kwargs["stdin"], subprocess.DEVNULL)

    def test_load_rejects_missing_non_regular_extra_channel_and_too_long_inputs(self) -> None:
        path = Path("audio.wav")
        with patch.object(Path, "exists", return_value=False):
            self.assertEqual(input_error_code(lambda: load_audio_input(path)), "missing_input")
        with (
            patch.object(Path, "exists", return_value=True),
            patch.object(Path, "is_file", return_value=False),
        ):
            self.assertEqual(input_error_code(lambda: load_audio_input(path)), "invalid_input_path")
        with (
            patch.object(Path, "exists", return_value=True),
            patch.object(Path, "is_file", return_value=True),
            patch("audio2haptic.core.preprocess.probe_audio", return_value=(3, 1_000)),
        ):
            self.assertEqual(input_error_code(lambda: load_audio_input(path)), "unsupported_channels")
        with (
            patch.object(Path, "exists", return_value=True),
            patch.object(Path, "is_file", return_value=True),
            patch("audio2haptic.core.preprocess.probe_audio", return_value=(1, MAX_INPUT_DURATION_MS)),
            patch(
                "audio2haptic.core.preprocess.decode_to_mono_samples",
                return_value=([0.0] * 160_001, 16_000, "test"),
            ),
        ):
            self.assertEqual(input_error_code(lambda: load_audio_input(path)), "input_too_long")

    def test_load_rejects_probed_overlimit_before_decoding(self) -> None:
        path = Path("audio.wav")
        with (
            patch.object(Path, "exists", return_value=True),
            patch.object(Path, "is_file", return_value=True),
            patch(
                "audio2haptic.core.preprocess.probe_audio",
                return_value=(1, MAX_INPUT_DURATION_MS + 1),
            ),
            patch("audio2haptic.core.preprocess.decode_to_mono_samples") as decode,
        ):
            self.assertEqual(input_error_code(lambda: load_audio_input(path)), "input_too_long")
        decode.assert_not_called()

    def test_load_accepts_the_exact_limit_and_describes_stereo_downmix(self) -> None:
        path = Path("audio.mp3")
        samples = [0.0] * 160_000
        with (
            patch.object(Path, "exists", return_value=True),
            patch.object(Path, "is_file", return_value=True),
            patch("audio2haptic.core.preprocess.probe_audio", return_value=(2, MAX_INPUT_DURATION_MS)),
            patch(
                "audio2haptic.core.preprocess.decode_to_mono_samples",
                return_value=(samples, 16_000, "ffmpeg"),
            ),
        ):
            audio = load_audio_input(path)
        self.assertEqual(audio.duration_ms, MAX_INPUT_DURATION_MS)
        self.assertEqual(audio.channel_policy, "downmix_to_mono")
        self.assertEqual(audio.input_format, "mp3")


if __name__ == "__main__":
    unittest.main()
