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

"""Public contract tests for the installed Audio2Haptic converter."""
# pylint: disable=duplicate-code

from __future__ import annotations

import json
import math
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
from typing import cast
import unittest
import wave
import contextlib
import errno
import io
from unittest.mock import patch

from audio2haptic.app.cli import EXIT_ARGUMENT_ERROR
from audio2haptic.app.cli import EXIT_CONVERSION_FAILED
from audio2haptic.app.cli import EXIT_SUCCESS
from audio2haptic.app.cli import main
from audio2haptic.app.convert import CleanupFailure
from audio2haptic.app.convert import convert
from audio2haptic.app.convert import PublicationError
from audio2haptic.core.compatibility import parse_openharmony_payload
from audio2haptic.core.compatibility import validate_openharmony_payload
from audio2haptic.core.models import ConversionRequest

SAMPLE_RATE = 8_000
LEFT_ONSET_MS = 200
RIGHT_ONSET_MS = 700
ACCENT_WIDTH_MS = 40


class FailingOutput:
    def __init__(self, output):
        self.output = output
        self.name = output.name

    def __enter__(self):
        return self

    def __exit__(self, *args):
        return self.output.__exit__(*args)

    def write(self, _content: bytes) -> None:
        raise OSError("simulated write failure")


def write_rhythmic_wav(path: Path) -> None:
    samples = []
    for index in range(SAMPLE_RATE):
        time_s = index / SAMPLE_RATE
        envelope = max(0.0, 1.0 - ((time_s * 4) % 1) * 16)
        samples.append(round(0.65 * envelope * math.sin(2 * math.pi * 180 * time_s) * 32767))
    output = cast(wave.Wave_write, wave.open(str(path), "wb"))
    with output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(SAMPLE_RATE)
        output.writeframes(struct.pack(f"<{len(samples)}h", *samples))


def write_rumble_wav(path: Path, duration_ms: int) -> None:
    sample_count = round(SAMPLE_RATE * duration_ms / 1_000)
    samples = [
        round(0.35 * math.sin(2 * math.pi * 78 * index / SAMPLE_RATE) * 32767)
        for index in range(sample_count)
    ]
    output = cast(wave.Wave_write, wave.open(str(path), "wb"))
    with output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(SAMPLE_RATE)
        output.writeframes(struct.pack(f"<{len(samples)}h", *samples))


def _decaying_accent_sample(offset: int) -> int:
    width = round(ACCENT_WIDTH_MS * SAMPLE_RATE / 1_000)
    if offset < 0 or offset >= width:
        return 0
    envelope = math.exp(-8 * offset / width)
    return round(0.9 * envelope * math.sin(math.tau * 180 * offset / SAMPLE_RATE) * 32767)


def write_stereo_call_response_wav(path: Path) -> None:
    samples = []
    left_start = round(LEFT_ONSET_MS * SAMPLE_RATE / 1_000)
    right_start = round(RIGHT_ONSET_MS * SAMPLE_RATE / 1_000)
    for index in range(SAMPLE_RATE):
        samples.extend(
            (
                _decaying_accent_sample(index - left_start),
                _decaying_accent_sample(index - right_start),
            )
        )
    output = cast(wave.Wave_write, wave.open(str(path), "wb"))
    with output:
        output.setnchannels(2)
        output.setsampwidth(2)
        output.setframerate(SAMPLE_RATE)
        output.writeframes(struct.pack(f"<{len(samples)}h", *samples))


def run(arguments: list[str]) -> tuple[int, str, str]:
    stdout = io.StringIO()
    stderr = io.StringIO()
    with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
        try:
            status = main(arguments)
        except SystemExit as exc:
            status = int(exc.code)
    return status, stdout.getvalue(), stderr.getvalue()


class ConverterContractTests(unittest.TestCase):
    def test_help_only_exposes_convert(self) -> None:
        status, output, error = run(["--help"])

        self.assertEqual(status, EXIT_SUCCESS, error)
        self.assertIn("convert", output)
        self.assertNotIn("inspect", output)
        self.assertNotIn("release-gate", output)
        self.assertNotIn("doctor", output)

    def test_wav_conversion_writes_json_without_sidecars(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_dir:
            temporary_path = Path(temporary_dir)
            input_path = temporary_path / "input.wav"
            output_path = temporary_path / "output.json"
            write_rhythmic_wav(input_path)

            status, output, error = run(["convert", str(input_path), "-o", str(output_path)])

            self.assertEqual(status, EXIT_SUCCESS, error)
            self.assertEqual(output.strip(), str(output_path))
            content = output_path.read_bytes()
            payload = json.loads(content)
            validate_openharmony_payload(payload, json_size_bytes=len(content))
            self.assertEqual(payload["Channels"][0]["Parameters"]["Index"], 0)
            self.assertFalse(output_path.with_name("output.report.json").exists())
            self.assertFalse(output_path.with_name("output.preview.json").exists())

    def test_html_is_direct_and_offline(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_dir:
            temporary_path = Path(temporary_dir)
            input_path = temporary_path / "input.wav"
            output_path = temporary_path / "output.json"
            html_path = temporary_path / "output.html"
            write_rhythmic_wav(input_path)

            status, _, error = run(["convert", str(input_path), "-o", str(output_path), "--html", str(html_path)])

            self.assertEqual(status, EXIT_SUCCESS, error)
            html = html_path.read_text(encoding="utf-8")
            self.assertIn("Audio2Haptic Visualization", html)
            self.assertIn("Audio waveform", html)
            self.assertIn("Haptic timeline", html)
            self.assertNotIn('src="http', html)
            self.assertNotIn('href="http', html)

    def test_continuous_bed_just_over_5000ms_is_decoder_valid(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_dir:
            temporary_path = Path(temporary_dir)
            input_path = temporary_path / "input.wav"
            output_path = temporary_path / "output.json"
            write_rumble_wav(input_path, duration_ms=5_004)

            result = convert(ConversionRequest(input_path, output_path))

            payload = json.loads(output_path.read_bytes())
            continuous = [
                item["Event"]
                for item in payload["Channels"][0]["Pattern"]
                if item["Event"]["Type"] == "continuous"
            ]
            self.assertGreater(result.event_count, 0)
            self.assertTrue(continuous)
            self.assertTrue(all(4 <= len(event["Parameters"]["Curve"]) <= 16 for event in continuous))
            validate_openharmony_payload(payload, json_size_bytes=output_path.stat().st_size)

    def test_mp3_conversion_uses_ffmpeg(self) -> None:
        self.assertIsNotNone(shutil.which("ffmpeg"), "ffmpeg is required for MP3 conversion")
        with tempfile.TemporaryDirectory() as temporary_dir:
            temporary_path = Path(temporary_dir)
            wav_path = temporary_path / "input.wav"
            mp3_path = temporary_path / "input.mp3"
            output_path = temporary_path / "output.json"
            write_rhythmic_wav(wav_path)
            encoded = subprocess.run(
                ["ffmpeg", "-y", "-loglevel", "error", "-i", str(wav_path), str(mp3_path)],
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )
            self.assertEqual(encoded.returncode, 0, encoded.stderr)

            status, _, error = run(["convert", str(mp3_path), "-o", str(output_path)])

            self.assertEqual(status, EXIT_SUCCESS, error)
            self.assertTrue(output_path.is_file())

    def test_stereo_call_response_downmix_keeps_both_channel_accents(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_dir:
            temporary_path = Path(temporary_dir)
            input_path = temporary_path / "input.wav"
            output_path = temporary_path / "output.json"
            write_stereo_call_response_wav(input_path)

            convert(ConversionRequest(input_path, output_path))

            payload = json.loads(output_path.read_bytes())
            events = parse_openharmony_payload(payload).require_events()
            transient_times = [event.start_ms for event in events if event.kind == "transient"]
            for expected_ms in (LEFT_ONSET_MS, RIGHT_ONSET_MS):
                self.assertTrue(any(abs(actual_ms - expected_ms) <= ACCENT_WIDTH_MS for actual_ms in transient_times))

    def test_conversion_accepts_a_local_file_named_like_a_protocol(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_dir:
            temporary_path = Path(temporary_dir)
            input_path = temporary_path / "file:input.wav"
            output_path = temporary_path / "output.json"
            write_rhythmic_wav(input_path)

            result = convert(ConversionRequest(input_path, output_path))

            self.assertEqual(result.output_path, output_path)
            self.assertTrue(output_path.is_file())

    def test_cli_rejects_unsupported_command_and_output_collisions(self) -> None:
        status, _, error = run(["doctor"])
        self.assertEqual(status, EXIT_ARGUMENT_ERROR)
        self.assertIn("invalid choice", error)

        with tempfile.TemporaryDirectory() as temporary_dir:
            temporary_path = Path(temporary_dir)
            input_path = temporary_path / "input.wav"
            output_path = temporary_path / "output.json"
            write_rhythmic_wav(input_path)
            output_path.write_text("preserve", encoding="utf-8")

            status, _, error = run(["convert", str(input_path), "-o", str(output_path)])
            self.assertEqual(status, EXIT_CONVERSION_FAILED)
            self.assertIn("output_already_exists", error)
            self.assertEqual(output_path.read_text(encoding="utf-8"), "preserve")

            status, _, error = run(["convert", str(input_path), "-o", str(input_path)])
            self.assertEqual(status, EXIT_CONVERSION_FAILED)
            self.assertIn("input_output_conflict", error)

            status, _, error = run(
                [
                    "convert",
                    str(input_path),
                    "-o",
                    str(temporary_path / "same.out"),
                    "--html",
                    str(temporary_path / "same.out"),
                ]
            )
            self.assertEqual(status, EXIT_CONVERSION_FAILED)
            self.assertIn("output_path_conflict", error)

    def test_atomic_publish_removes_partial_output_after_link_failure(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_dir:
            temporary_path = Path(temporary_dir)
            input_path = temporary_path / "input.wav"
            output_path = temporary_path / "output.json"
            html_path = temporary_path / "output.html"
            write_rhythmic_wav(input_path)
            original_link = os.link
            attempts = 0

            def fail_second_link(source, target):
                nonlocal attempts
                attempts += 1
                if attempts == 2:
                    raise OSError(errno.EIO, "simulated link failure")
                return original_link(source, target)

            with self.assertRaises(OSError):
                with patch("audio2haptic.app.convert.os.link", side_effect=fail_second_link):
                    convert(ConversionRequest(input_path, output_path), html_path=html_path)

            self.assertFalse(output_path.exists())
            self.assertFalse(html_path.exists())
            self.assertFalse(any(temporary_path.glob(".output.*")))

    def test_atomic_publish_removes_a_temp_file_after_its_write_fails(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_dir:
            temporary_path = Path(temporary_dir)
            input_path = temporary_path / "input.wav"
            output_path = temporary_path / "output.json"
            write_rhythmic_wav(input_path)
            named_temporary_file = tempfile.NamedTemporaryFile

            def failing_temporary_file(*args, **kwargs):
                return FailingOutput(named_temporary_file(*args, **kwargs))

            with self.assertRaises(OSError):
                with patch(
                    "audio2haptic.app.convert.tempfile.NamedTemporaryFile",
                    side_effect=failing_temporary_file,
                ):
                    convert(ConversionRequest(input_path, output_path))

            self.assertFalse(any(temporary_path.glob(".output.*")))
            self.assertFalse(output_path.exists())

    def test_publish_rollback_preserves_replaced_target(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_dir:
            temporary_path = Path(temporary_dir)
            input_path = temporary_path / "input.wav"
            output_path = temporary_path / "output.json"
            html_path = temporary_path / "output.html"
            replacement_path = temporary_path / "replacement.json"
            write_rhythmic_wav(input_path)
            replacement_path.write_text("replacement", encoding="utf-8")
            original_link = os.link

            def replace_first_target(source, target):
                original_link(source, target)
                if target == output_path:
                    os.replace(replacement_path, output_path)
                    html_path.write_text("other-writer", encoding="utf-8")

            with self.assertRaises(FileExistsError):
                with patch("audio2haptic.app.convert.os.link", side_effect=replace_first_target):
                    convert(ConversionRequest(input_path, output_path), html_path=html_path)

            self.assertEqual(output_path.read_text(encoding="utf-8"), "replacement")
            self.assertEqual(html_path.read_text(encoding="utf-8"), "other-writer")

    def test_publish_failure_reports_an_output_that_could_not_be_rolled_back(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_dir:
            temporary_path = Path(temporary_dir)
            input_path = temporary_path / "input.wav"
            output_path = temporary_path / "output.json"
            html_path = temporary_path / "output.html"
            write_rhythmic_wav(input_path)
            original_link = os.link
            original_unlink = Path.unlink
            attempts = 0

            def fail_second_link(source, target):
                nonlocal attempts
                attempts += 1
                if attempts == 2:
                    raise OSError(errno.EIO, "simulated link failure")
                return original_link(source, target)

            def fail_output_unlink(path, *args, **kwargs):
                if path == output_path:
                    raise OSError(errno.EACCES, "simulated rollback failure")
                return original_unlink(path, *args, **kwargs)

            with self.assertRaises(PublicationError) as context:
                with (
                    patch("audio2haptic.app.convert.os.link", side_effect=fail_second_link),
                    patch(
                        "audio2haptic.app.convert.Path.unlink",
                        autospec=True,
                        side_effect=fail_output_unlink,
                    ),
                ):
                    convert(ConversionRequest(input_path, output_path), html_path=html_path)

            self.assertIn(str(output_path), str(context.exception))
            self.assertIn(f"[Errno {errno.EIO}]", str(context.exception))
            self.assertIn(f"[Errno {errno.EACCES}]", str(context.exception))
            self.assertIn("simulated rollback failure", str(context.exception))
            self.assertTrue(output_path.exists())

    def test_cli_reports_os_error_number_without_a_traceback(self) -> None:
        failure = OSError(errno.ENOSPC, "simulated full filesystem")
        with patch("audio2haptic.app.cli.convert", side_effect=failure):
            status, _, error = run(["convert", "input.wav", "-o", "output.json"])

        self.assertEqual(status, EXIT_CONVERSION_FAILED)
        self.assertIn(f"[Errno {errno.ENOSPC}]", error)
        self.assertIn("simulated full filesystem", error)
        self.assertNotIn("Traceback", error)

    def test_cli_reports_publish_and_rollback_os_error_numbers(self) -> None:
        failure = PublicationError(
            OSError(errno.EIO, "simulated publish failure"),
            [CleanupFailure(Path("output.json"), OSError(errno.EACCES, "simulated rollback failure"))],
        )
        with patch("audio2haptic.app.cli.convert", side_effect=failure):
            status, _, error = run(["convert", "input.wav", "-o", "output.json"])

        self.assertEqual(status, EXIT_CONVERSION_FAILED)
        self.assertIn(f"[Errno {errno.EIO}]", error)
        self.assertIn(f"[Errno {errno.EACCES}]", error)
        self.assertIn("output.json", error)


if __name__ == "__main__":
    unittest.main()
