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

"""Exercise the installed converter with generated local audio."""

from __future__ import annotations

import json
import math
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
from typing import cast
import wave

SMOKE_SAMPLE_RATE = 8_000
SMOKE_AMPLITUDE = 0.55
SMOKE_TONE_FREQUENCY_HZ = 180
PCM_SAMPLE_WIDTH_BYTES = 2
PCM_SIGNED_16_PEAK = 32_767
MP3_ENCODE_TIMEOUT_SECONDS = 30
CONVERSION_TIMEOUT_SECONDS = 60
EXIT_SUCCESS = 0
EXIT_FAILURE = 1


def write_wav(path: Path) -> None:
    samples = [
        round(
            SMOKE_AMPLITUDE
            * math.sin(math.tau * SMOKE_TONE_FREQUENCY_HZ * index / SMOKE_SAMPLE_RATE)
            * PCM_SIGNED_16_PEAK
        )
        for index in range(SMOKE_SAMPLE_RATE)
    ]
    output = cast(wave.Wave_write, wave.open(str(path), "wb"))
    with output:
        output.setnchannels(1)
        output.setsampwidth(PCM_SAMPLE_WIDTH_BYTES)
        output.setframerate(SMOKE_SAMPLE_RATE)
        output.writeframes(struct.pack(f"<{len(samples)}h", *samples))


def installed_command() -> list[str]:
    executable = os.environ.get("AUDIO2HAPTIC_CLI", "audio2haptic")
    resolved = shutil.which(executable)
    if not resolved:
        raise RuntimeError("audio2haptic is not installed or is not on PATH.")
    return [resolved]


def encode_mp3(wav_path: Path, mp3_path: Path) -> None:
    result = subprocess.run(
        ["ffmpeg", "-y", "-loglevel", "error", "-i", str(wav_path), str(mp3_path)],
        capture_output=True,
        text=True,
        timeout=MP3_ENCODE_TIMEOUT_SECONDS,
        check=False,
    )
    if result.returncode:
        raise RuntimeError(result.stderr or "ffmpeg could not encode the MP3 smoke input.")


def run_convert(command: list[str], input_path: Path, output_path: Path, html_path: Path | None = None) -> None:
    arguments = command + ["convert", str(input_path), "-o", str(output_path)]
    if html_path is not None:
        arguments.extend(["--html", str(html_path)])
    result = subprocess.run(
        arguments,
        capture_output=True,
        text=True,
        timeout=CONVERSION_TIMEOUT_SECONDS,
        check=False,
    )
    if result.returncode:
        raise RuntimeError(result.stderr or result.stdout or "audio2haptic conversion failed.")
    if not output_path.is_file():
        raise RuntimeError("audio2haptic did not create the JSON output.")
    if html_path is not None and not html_path.is_file():
        raise RuntimeError("audio2haptic did not create the HTML output.")
    payload = json.loads(output_path.read_text(encoding="utf-8"))
    if not isinstance(payload.get("Channels"), list):
        raise RuntimeError("audio2haptic output does not contain Channels.")


def main() -> int:
    try:
        command = installed_command()
        with tempfile.TemporaryDirectory(prefix="audio2haptic-smoke-") as temporary_dir:
            temporary_path = Path(temporary_dir)
            wav_path = temporary_path / "input.wav"
            mp3_path = temporary_path / "input.mp3"
            write_wav(wav_path)
            encode_mp3(wav_path, mp3_path)
            run_convert(command, wav_path, temporary_path / "wav.json", temporary_path / "wav.html")
            run_convert(command, mp3_path, temporary_path / "mp3.json")
    except (OSError, RuntimeError, subprocess.TimeoutExpired, json.JSONDecodeError) as exc:
        print(exc, file=sys.stderr)
        return EXIT_FAILURE
    return EXIT_SUCCESS


if __name__ == "__main__":
    raise SystemExit(main())
