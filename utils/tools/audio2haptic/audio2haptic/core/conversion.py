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

from dataclasses import dataclass

from .device_profile import resolve_device_profile
from .dsp import generate_candidate_events
from .models import AudioInput, ConversionRequest, HapticEvent
from .optimizer import optimize_events
from .preprocess import load_audio_input


@dataclass(frozen=True)
class PreparedDspConversion:
    audio: AudioInput
    events: list[HapticEvent]


def prepare_dsp_conversion(request: ConversionRequest) -> PreparedDspConversion:
    audio = load_audio_input(request.input_path)
    profile = resolve_device_profile(request.device_profile)
    candidates = generate_candidate_events(audio, profile=profile)
    events, _ = optimize_events(candidates)
    return PreparedDspConversion(
        audio=audio,
        events=events,
    )
