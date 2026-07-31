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

from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path


class StructuralRole(str, Enum):
    ACCENT = "accent"
    BED = "bed"


class HapticTone(str, Enum):
    NEUTRAL = "neutral"
    SOFT_ELASTIC = "soft_elastic"
    TENSE = "tense"
    HEAVY = "heavy"


@dataclass
class AudioInput:  # pylint: disable=too-many-instance-attributes
    path: Path
    input_format: str
    duration_ms: int
    sample_rate: int
    input_channels: int
    channel_policy: str
    samples: list[float]
    decode_backend: str


@dataclass(frozen=True)
class ConversionRequest:
    input_path: Path
    output_path: Path
    device_profile: str = "default"


@dataclass
class CurvePoint:
    time_ms: int
    intensity: int
    frequency: int | None = None


@dataclass
class HapticEvent:  # pylint: disable=too-many-instance-attributes
    start_ms: int
    kind: str
    intensity: int
    source: str
    duration_ms: int | None = None
    frequency: int | None = None
    curve: list[CurvePoint] = field(default_factory=list)
    tags: list[str] = field(default_factory=list)
    structural_role: StructuralRole | None = None
    haptic_tone: HapticTone | None = None
