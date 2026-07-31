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
from .openharmony_contract import CHANNEL_COUNT
from .openharmony_contract import DEFAULT_CHANNEL_INDEX
from .openharmony_contract import FORMAT_VERSION
from .openharmony_contract import MAX_CURVE_FREQUENCY
from .openharmony_contract import MAX_EVENT_FREQUENCY
from .openharmony_contract import MAX_EVENT_INTENSITY
from .openharmony_contract import MIN_CURVE_FREQUENCY
from .openharmony_contract import MIN_EVENT_FREQUENCY
from .openharmony_contract import MIN_EVENT_INTENSITY
from .openharmony_contract import TRANSIENT_DURATION_MS

CURVE_INTENSITY_PRECISION = 2
FALLBACK_EVENT_FREQUENCY = 50


def render_openharmony(source_events: list[HapticEvent]) -> dict:
    events = [_render_event(event) for event in source_events]
    return {
        "MetaData": {
            "Version": FORMAT_VERSION,
            "ChannelNumber": CHANNEL_COUNT,
        },
        "Channels": [
            {
                "Parameters": {"Index": DEFAULT_CHANNEL_INDEX},
                "Pattern": events,
            }
        ],
    }


def _render_event(event: HapticEvent) -> dict:
    event_frequency = int(event.frequency if event.frequency is not None else FALLBACK_EVENT_FREQUENCY)
    source_peak = max((point.intensity for point in event.curve), default=event.intensity)
    curve_peak = max(MIN_EVENT_INTENSITY, min(MAX_EVENT_INTENSITY, source_peak))
    parameters = {
        "Intensity": int(curve_peak),
        "Frequency": int(max(MIN_EVENT_FREQUENCY, min(MAX_EVENT_FREQUENCY, event_frequency))),
    }
    if event.kind == "continuous" and event.curve:
        parameters["Curve"] = [
            {
                "Time": point.time_ms,
                "Intensity": _relative_curve_intensity(point.intensity, curve_peak),
                "Frequency": _relative_curve_frequency(point.frequency, event_frequency),
            }
            for point in event.curve
        ]

    payload = {
        "Type": event.kind,
        "StartTime": int(event.start_ms),
        "Duration": int(event.duration_ms if event.duration_ms is not None else TRANSIENT_DURATION_MS),
        "Parameters": parameters,
    }
    return {"Event": payload}


def _relative_curve_frequency(point_frequency: int | None, event_frequency: int) -> int:
    absolute_frequency = event_frequency if point_frequency is None else point_frequency
    return max(
        MIN_CURVE_FREQUENCY,
        min(MAX_CURVE_FREQUENCY, int(absolute_frequency) - event_frequency),
    )


def _relative_curve_intensity(point_intensity: int, curve_peak: int) -> float:
    if curve_peak <= 0:
        return 0.0
    bounded_intensity = max(MIN_EVENT_INTENSITY, min(curve_peak, point_intensity))
    return round(bounded_intensity / curve_peak, CURVE_INTENSITY_PRECISION)
