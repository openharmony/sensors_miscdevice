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

from .models import HapticEvent
from .openharmony_contract import MAX_PATTERN_EVENT_COUNT
from .playback import plan_packet_safe_events
from .serialization import encode_json


def optimize_events(candidate_events: list[HapticEvent]) -> tuple[list[HapticEvent], int]:
    continuous = [event for event in candidate_events if event.kind == "continuous"]
    transient = [event for event in candidate_events if event.kind == "transient"]

    continuous = sorted(continuous, key=lambda event: (-event.intensity, event.start_ms))
    transient = sorted(transient, key=lambda event: event.start_ms)

    kept: list[HapticEvent] = []
    dropped = 0
    for event in transient:
        if len(kept) >= MAX_PATTERN_EVENT_COUNT:
            dropped += 1
            continue
        kept.append(event)

    continuous_budget = MAX_PATTERN_EVENT_COUNT - len(kept)
    kept.extend(continuous[:continuous_budget])
    dropped += max(0, len(continuous) - continuous_budget)

    kept, packet_dropped = plan_packet_safe_events(kept, MAX_PATTERN_EVENT_COUNT)
    return kept, dropped + packet_dropped


def estimate_json_size_bytes(payload: dict) -> int:
    return len(encode_json(payload))
