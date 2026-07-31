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

from __future__ import annotations

import json
import math
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

import numpy as np

from .models import AudioInput

SUPPORTED_INPUT_SUFFIXES = {".wav", ".mp3"}
TARGET_SAMPLE_RATE = 16_000
MAX_INPUT_DURATION_MS = 10_000
MILLISECONDS_PER_SECOND = 1_000
INPUT_PROBE_TIMEOUT_SECONDS = 10
INPUT_DECODE_TIMEOUT_SECONDS = 15
MAX_SUPPORTED_INPUT_CHANNELS = 2
PCM_SAMPLE_WIDTH_BYTES = 2
PCM_SIGNED_16_SCALE = 32_768.0
DECODE_DURATION_GUARD_MS = 1
DECODED_SAMPLE_GUARD_COUNT = 1
MAX_INPUT_DURATION_SECONDS = MAX_INPUT_DURATION_MS // MILLISECONDS_PER_SECOND
MAX_DECODED_SAMPLE_COUNT = TARGET_SAMPLE_RATE * MAX_INPUT_DURATION_MS // MILLISECONDS_PER_SECOND
MAX_DECODED_PCM_BYTES = (MAX_DECODED_SAMPLE_COUNT + DECODED_SAMPLE_GUARD_COUNT) * PCM_SAMPLE_WIDTH_BYTES
MAX_PROBE_OUTPUT_BYTES = 64 * 1024
LOCAL_PROTOCOLS = "file"


class InputError(RuntimeError):
    def __init__(self, stage: str, code: str, message: str):
        super().__init__(message)
        self.stage = stage
        self.code = code
        self.message = message


def ensure_supported_suffix(path: Path) -> str:
    suffix = path.suffix.lower()
    if suffix not in SUPPORTED_INPUT_SUFFIXES:
        raise InputError(
            "input_format",
            "unsupported_format",
            "Only wav and mp3 inputs are supported in the current version.",
        )
    return suffix.lstrip(".")


def probe_audio(path: Path) -> tuple[int, int]:
    ffprobe = shutil.which("ffprobe")
    if not ffprobe:
        raise InputError(
            "input_probe",
            "ffprobe_missing",
            "ffprobe is required to inspect input audio.",
        )
    return _probe_metadata(_run_ffprobe(_ffprobe_command(ffprobe, path)))


def _ffprobe_command(ffprobe: str, path: Path) -> list[str]:
    return [
        ffprobe,
        "-protocol_whitelist",
        LOCAL_PROTOCOLS,
        "-v",
        "error",
        "-select_streams",
        "a:0",
        "-show_entries",
        "stream=channels",
        "-show_entries",
        "format=duration",
        "-of",
        "json",
        _input_uri(path),
    ]


def _run_ffprobe(command: list[str]) -> subprocess.CompletedProcess:
    try:
        with tempfile.TemporaryFile(mode="w+t", encoding="utf-8") as output:
            result = subprocess.run(
                command,
                stdout=output,
                stderr=subprocess.DEVNULL,
                stdin=subprocess.DEVNULL,
                text=True,
                timeout=INPUT_PROBE_TIMEOUT_SECONDS,
                check=False,
            )
            output.seek(0, os.SEEK_END)
            if output.tell() > MAX_PROBE_OUTPUT_BYTES:
                raise InputError(
                    "input_probe",
                    "probe_output_too_large",
                    "ffprobe returned more metadata than the input policy permits.",
                )
            output.seek(0)
            result.stdout = output.read() or result.stdout
    except subprocess.TimeoutExpired as exc:
        raise InputError(
            "input_probe",
            "probe_timed_out",
            "Timed out while inspecting the audio input.",
        ) from exc
    except OSError as exc:
        raise InputError(
            "input_probe",
            "probe_failed",
            f"Failed to inspect the audio input with ffprobe: {exc}",
        ) from exc
    if result.returncode != 0:
        raise InputError(
            "input_probe",
            "probe_failed",
            "Failed to inspect the audio input with ffprobe.",
        )
    return result


def _probe_metadata(result: subprocess.CompletedProcess) -> tuple[int, int]:
    try:
        payload = json.loads(result.stdout or "{}")
    except json.JSONDecodeError as exc:
        raise InputError(
            "input_probe",
            "probe_failed",
            "ffprobe returned an invalid response.",
        ) from exc
    if not isinstance(payload, dict):
        raise InputError(
            "input_probe",
            "probe_failed",
            "ffprobe returned an invalid response.",
        )
    streams = payload.get("streams", [])
    fmt = payload.get("format", {})
    if not isinstance(streams, list) or not isinstance(fmt, dict):
        raise InputError(
            "input_probe",
            "probe_failed",
            "ffprobe returned an invalid response.",
        )
    channels = 0
    try:
        for stream in streams:
            if isinstance(stream, dict):
                channels = max(channels, int(stream.get("channels", 0) or 0))
        duration_seconds = float(fmt.get("duration", 0.0) or 0.0)
    except (TypeError, ValueError) as exc:
        raise InputError(
            "input_probe",
            "probe_failed",
            "ffprobe returned invalid audio metadata.",
        ) from exc
    if not math.isfinite(duration_seconds) or duration_seconds <= 0:
        raise InputError(
            "input_probe",
            "invalid_duration",
            "Input audio does not expose a valid duration.",
        )
    duration_ms = int(math.ceil(duration_seconds * MILLISECONDS_PER_SECOND))
    if channels <= 0:
        raise InputError(
            "input_decode",
            "invalid_channels",
            "Input audio does not expose a valid channel count.",
        )
    return channels, duration_ms


def decode_to_mono_samples(path: Path) -> tuple[list[float], int, str]:
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        raise InputError(
            "input_decode",
            "ffmpeg_missing",
            "ffmpeg is required to decode input audio.",
        )
    raw_samples = _run_ffmpeg(_ffmpeg_command(ffmpeg, path))
    samples = np.frombuffer(raw_samples, dtype=np.int16).astype(np.float32) / PCM_SIGNED_16_SCALE
    if samples.size == 0:
        raise InputError("input_decode", "empty_audio", "Decoded audio is empty.")
    return samples.tolist(), TARGET_SAMPLE_RATE, "ffmpeg"


def _ffmpeg_command(ffmpeg: str, path: Path) -> list[str]:
    return [
        ffmpeg,
        "-nostdin",
        "-protocol_whitelist",
        LOCAL_PROTOCOLS,
        "-v",
        "error",
        "-i",
        _input_uri(path),
        "-t",
        str((MAX_INPUT_DURATION_MS + DECODE_DURATION_GUARD_MS) / MILLISECONDS_PER_SECOND),
        "-f",
        "s16le",
        "-acodec",
        "pcm_s16le",
        "-ac",
        "1",
        "-ar",
        str(TARGET_SAMPLE_RATE),
        "-",
    ]


def _run_ffmpeg(command: list[str]) -> bytes:
    try:
        result = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            stdin=subprocess.DEVNULL,
            timeout=INPUT_DECODE_TIMEOUT_SECONDS,
            check=False,
        )
    except subprocess.TimeoutExpired as exc:
        raise InputError(
            "input_decode",
            "decode_timed_out",
            "Timed out while decoding the audio input.",
        ) from exc
    except OSError as exc:
        raise InputError(
            "input_decode",
            "decode_failed",
            f"Failed to decode the input audio: {exc}",
        ) from exc
    if result.returncode != 0:
        raise InputError(
            "input_decode",
            "decode_failed",
            "Failed to decode the input audio.",
        )
    raw_samples = result.stdout or b""
    if len(raw_samples) > MAX_DECODED_PCM_BYTES:
        raise InputError(
            "input_duration",
            "input_too_long",
            f"Input audio exceeds the {MAX_INPUT_DURATION_SECONDS}s limit for the current version.",
        )
    return raw_samples


def _input_uri(path: Path) -> str:
    return f"file:{path.resolve()}"


def load_audio_input(path: Path) -> AudioInput:
    input_format = ensure_supported_suffix(path)
    if not path.exists():
        raise InputError("input_path", "missing_input", "Input audio path does not exist.")
    if not path.is_file():
        raise InputError(
            "input_path",
            "invalid_input_path",
            "Input audio path must be a regular file.",
        )
    channels, probed_duration_ms = probe_audio(path)
    if channels > MAX_SUPPORTED_INPUT_CHANNELS:
        raise InputError(
            "input_channels",
            "unsupported_channels",
            "Only mono and stereo inputs are supported.",
        )
    if probed_duration_ms > MAX_INPUT_DURATION_MS:
        raise InputError(
            "input_duration",
            "input_too_long",
            f"Input audio exceeds the {MAX_INPUT_DURATION_SECONDS}s limit for the current version.",
        )
    samples, sample_rate, backend = decode_to_mono_samples(path)
    if len(samples) > MAX_DECODED_SAMPLE_COUNT:
        raise InputError(
            "input_duration",
            "input_too_long",
            f"Input audio exceeds the {MAX_INPUT_DURATION_SECONDS}s limit for the current version.",
        )
    decoded_duration_ms = int(math.ceil((len(samples) * MILLISECONDS_PER_SECOND) / max(sample_rate, 1)))
    return AudioInput(
        path=path,
        input_format=input_format,
        duration_ms=decoded_duration_ms,
        sample_rate=sample_rate,
        input_channels=channels,
        channel_policy="mono_passthrough" if channels == 1 else "downmix_to_mono",
        samples=samples,
        decode_backend=backend,
    )
