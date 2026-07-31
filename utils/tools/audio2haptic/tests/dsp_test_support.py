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

import math
from pathlib import Path

import numpy as np

from audio2haptic.core.models import AudioInput


SAMPLE_RATE = 16_000


def audio_input(samples: np.ndarray, *, path_name: str = "synthetic.wav") -> AudioInput:
    return AudioInput(
        path=Path(path_name),
        input_format="wav",
        duration_ms=round(samples.size * 1000 / SAMPLE_RATE),
        sample_rate=SAMPLE_RATE,
        input_channels=1,
        channel_policy="mono_passthrough",
        samples=samples.astype(np.float32).tolist(),
        decode_backend="synthetic",
    )


def add_decaying_accent(
    samples: np.ndarray,
    start_ms: int,
    amplitude: float,
    *,
    width_ms: int = 36,
    clip_to_buffer: bool,
) -> None:
    start = round(start_ms * SAMPLE_RATE / 1000)
    width = round(width_ms * SAMPLE_RATE / 1000)
    time = np.arange(width, dtype=np.float32) / SAMPLE_RATE
    envelope = np.exp(-8 * np.arange(width, dtype=np.float32) / max(width, 1))
    pulse = amplitude * envelope * np.sin(2 * math.pi * 180 * time)
    if clip_to_buffer:
        samples[start:start + width] += pulse[:max(0, samples.size - start)]
    else:
        samples[start:start + width] += pulse
