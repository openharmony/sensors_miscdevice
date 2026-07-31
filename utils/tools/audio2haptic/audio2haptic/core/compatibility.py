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
from dataclasses import dataclass

from .models import CurvePoint, HapticEvent
from .openharmony_contract import CHANNEL_COUNT
from .openharmony_contract import DEFAULT_CHANNEL_INDEX
from .openharmony_contract import FORMAT_VERSION
from .openharmony_contract import MAX_CONTINUOUS_DURATION_MS
from .openharmony_contract import MAX_CURVE_FREQUENCY
from .openharmony_contract import MAX_CURVE_INTENSITY
from .openharmony_contract import MAX_CURVE_POINT_COUNT
from .openharmony_contract import MAX_EVENT_FREQUENCY
from .openharmony_contract import MAX_EVENT_INTENSITY
from .openharmony_contract import MAX_EVENT_START_TIME_MS
from .openharmony_contract import MAX_JSON_SIZE_BYTES
from .openharmony_contract import MAX_PATTERN_EVENT_COUNT
from .openharmony_contract import MIN_CURVE_FREQUENCY
from .openharmony_contract import MIN_CURVE_INTENSITY
from .openharmony_contract import MIN_CURVE_POINT_COUNT
from .openharmony_contract import MIN_EVENT_FREQUENCY
from .openharmony_contract import MIN_EVENT_INTENSITY
from .openharmony_contract import TRANSIENT_DURATION_MS
from .optimizer import estimate_json_size_bytes


class CompatibilityError(RuntimeError):
    def __init__(self, code: str, message: str):
        super().__init__(message)
        self.code = code
        self.message = message


@dataclass(frozen=True)
class ParsedOpenHarmonyPayload:
    events: list[HapticEvent] | None
    compatibility_error: CompatibilityError | None
    readability_error: CompatibilityError | None

    def require_events(self) -> list[HapticEvent]:
        if self.events is not None:
            return self.events
        error = self.compatibility_error or self.readability_error
        if error is None:
            raise CompatibilityError(
                "invalid_output_structure",
                "OpenHarmony payload does not contain readable events.",
            )
        raise error


class _PayloadAnalysis:
    def __init__(self) -> None:
        self.compatibility_error: CompatibilityError | None = None
        self.readability_error: CompatibilityError | None = None
        self.events: list[HapticEvent] = []

    def compatibility_issue(self, code: str, message: str) -> None:
        if self.compatibility_error is None:
            self.compatibility_error = CompatibilityError(code, message)

    def unreadable(self, message: str) -> None:
        if self.readability_error is None:
            self.readability_error = CompatibilityError("invalid_output_structure", message)

    def result(self) -> ParsedOpenHarmonyPayload:
        events = (
            None
            if self.readability_error is not None
            else sorted(self.events, key=lambda event: event.start_ms)
        )
        return ParsedOpenHarmonyPayload(events, self.compatibility_error, self.readability_error)


def parse_openharmony_payload(
    payload: object,
    *,
    json_size_bytes: int | None = None,
) -> ParsedOpenHarmonyPayload:
    analysis = _PayloadAnalysis()
    if not isinstance(payload, dict):
        analysis.compatibility_issue("invalid_payload", "OpenHarmony payload must be an object.")
        analysis.unreadable("OpenHarmony payload must be an object.")
        return analysis.result()

    _record_metadata_issues(payload, analysis)
    pattern = _pattern_from_payload(payload, analysis)
    if pattern is None:
        return analysis.result()
    if len(pattern) > MAX_PATTERN_EVENT_COUNT:
        analysis.compatibility_issue(
            "too_many_events",
            f"Pattern exceeds the {MAX_PATTERN_EVENT_COUNT}-event limit.",
        )

    for item in pattern:
        event = _parse_event(item, analysis)
        if event is None:
            return analysis.result()
        analysis.events.append(event)

    actual_size = (
        estimate_json_size_bytes(payload)
        if json_size_bytes is None
        else json_size_bytes
    )
    if actual_size > MAX_JSON_SIZE_BYTES:
        analysis.compatibility_issue(
            "json_too_large",
            f"Rendered JSON exceeds the {MAX_JSON_SIZE_BYTES}-byte OpenHarmony decoder limit.",
        )
    return analysis.result()


def validate_openharmony_payload(
    payload: object,
    *,
    json_size_bytes: int | None = None,
) -> None:
    parsed = parse_openharmony_payload(payload, json_size_bytes=json_size_bytes)
    if parsed.compatibility_error is not None:
        raise parsed.compatibility_error


def _record_metadata_issues(payload: dict, analysis: _PayloadAnalysis) -> None:
    metadata = payload.get("MetaData")
    if not isinstance(metadata, dict):
        analysis.compatibility_issue("missing_metadata", "MetaData must be present.")
        return
    if metadata.get("Version") != FORMAT_VERSION:
        analysis.compatibility_issue("invalid_version", f"MetaData.Version must equal {FORMAT_VERSION}.")
    if metadata.get("ChannelNumber") != CHANNEL_COUNT:
        analysis.compatibility_issue(
            "invalid_channel_number",
            f"Current version only supports ChannelNumber={CHANNEL_COUNT}.",
        )


def _pattern_from_payload(payload: dict, analysis: _PayloadAnalysis) -> list | None:
    channels = payload.get("Channels")
    if not isinstance(channels, list) or len(channels) != CHANNEL_COUNT:
        analysis.compatibility_issue(
            "invalid_channels",
            "Current version requires exactly one channel.",
        )
        analysis.unreadable("Channels must contain exactly one channel object.")
        return None
    channel = channels[0]
    if not isinstance(channel, dict):
        analysis.compatibility_issue("invalid_channel", "Channel must be an object.")
        analysis.unreadable("Channels must contain exactly one channel object.")
        return None

    parameters = channel.get("Parameters", {})
    if not isinstance(parameters, dict):
        analysis.compatibility_issue(
            "invalid_channel_parameters",
            "Channel.Parameters must be an object.",
        )
    elif parameters.get("Index") != DEFAULT_CHANNEL_INDEX:
        analysis.compatibility_issue(
            "invalid_channel_index",
            f"Current version renders only channel index {DEFAULT_CHANNEL_INDEX}.",
        )

    pattern = channel.get("Pattern")
    if not isinstance(pattern, list):
        analysis.compatibility_issue("invalid_pattern", "Pattern must be a list.")
        analysis.unreadable("Channel.Pattern must be a list.")
        return None
    return pattern


def _parse_event(item: object, analysis: _PayloadAnalysis) -> HapticEvent | None:
    if not isinstance(item, dict):
        analysis.compatibility_issue("invalid_pattern_item", "Each Pattern item must be an object.")
        analysis.unreadable("Each Pattern item must contain an Event object.")
        return None
    event = item.get("Event")
    if not isinstance(event, dict):
        analysis.compatibility_issue(
            "missing_event",
            "Each Pattern item must contain an Event object.",
        )
        analysis.unreadable("Each Pattern item must contain an Event object.")
        return None

    event_type, start_time, duration = _record_event_header_issues(event, analysis)
    parameters = event.get("Parameters")
    if not isinstance(parameters, dict):
        analysis.compatibility_issue("missing_parameters", "Event.Parameters must be present.")
        analysis.unreadable("Event.Parameters must be an object.")
        return None
    intensity = parameters.get("Intensity")
    frequency = parameters.get("Frequency")
    if not isinstance(intensity, int) or not MIN_EVENT_INTENSITY <= intensity <= MAX_EVENT_INTENSITY:
        analysis.compatibility_issue(
            "invalid_intensity",
            f"Intensity must be within {MIN_EVENT_INTENSITY}..{MAX_EVENT_INTENSITY}.",
        )
    if not isinstance(frequency, int) or not MIN_EVENT_FREQUENCY <= frequency <= MAX_EVENT_FREQUENCY:
        analysis.compatibility_issue(
            "invalid_frequency",
            f"Frequency must be within {MIN_EVENT_FREQUENCY}..{MAX_EVENT_FREQUENCY}.",
        )
    scalar_values = (start_time, duration, intensity, frequency)
    if (
        not all(isinstance(value, int) for value in scalar_values)
        or not isinstance(event_type, str)
    ):
        analysis.unreadable("Event fields must use their declared scalar types.")
        return None

    return _build_event(event, analysis, (event_type, start_time, duration, intensity, frequency))


def _build_event(
    event: dict,
    analysis: _PayloadAnalysis,
    fields: tuple[object, object, object, object, object],
) -> HapticEvent | None:
    event_type, start_time, duration, intensity, frequency = fields
    curve = _parse_curve(event.get("Parameters", {}).get("Curve"), event, analysis)
    if curve is None:
        return None
    return HapticEvent(
        start_ms=start_time,
        kind=event_type,
        duration_ms=duration,
        intensity=intensity,
        frequency=frequency,
        source="rendered",
        curve=curve,
    )


def _record_event_header_issues(
    event: dict,
    analysis: _PayloadAnalysis,
) -> tuple[object, object, object]:
    event_type = event.get("Type")
    if event_type not in {"continuous", "transient"}:
        analysis.compatibility_issue(
            "invalid_event_type",
            "Event.Type must be continuous or transient.",
        )
    start_time = event.get("StartTime")
    if not isinstance(start_time, int):
        analysis.compatibility_issue("invalid_start_time", "Event.StartTime is out of range.")
    elif start_time < 0 or start_time > MAX_EVENT_START_TIME_MS:
        analysis.compatibility_issue("invalid_start_time", "Event.StartTime is out of range.")
    duration = event.get("Duration")
    if event_type == "transient":
        return event_type, start_time, TRANSIENT_DURATION_MS
    if not isinstance(duration, int):
        analysis.compatibility_issue("invalid_duration", "Event.Duration must be an integer.")
    elif event_type == "continuous" and (duration < 0 or duration > MAX_CONTINUOUS_DURATION_MS):
        analysis.compatibility_issue(
            "invalid_continuous_duration",
            f"Continuous duration must be within 0..{MAX_CONTINUOUS_DURATION_MS} ms.",
        )
    return event_type, start_time, duration


def _parse_curve(
    curve: object,
    event: dict,
    analysis: _PayloadAnalysis,
) -> list[CurvePoint] | None:
    if event.get("Type") == "transient":
        return []
    if curve is None:
        return []
    if event["Type"] == "continuous" and (
        not isinstance(curve, list)
        or len(curve) < MIN_CURVE_POINT_COUNT
        or len(curve) > MAX_CURVE_POINT_COUNT
    ):
        analysis.compatibility_issue(
            "invalid_curve_size",
            f"Continuous curve must contain {MIN_CURVE_POINT_COUNT}..{MAX_CURVE_POINT_COUNT} points.",
        )
    if not isinstance(curve, list):
        analysis.unreadable("Continuous Curve must be a list.")
        return None

    points: list[CurvePoint] = []
    for point in curve:
        parsed = _parse_curve_point(point, event, analysis)
        if parsed is None:
            return None
        points.append(parsed)
    return points


def _parse_curve_point(
    point: object,
    event: dict,
    analysis: _PayloadAnalysis,
) -> CurvePoint | None:
    event_type = event["Type"]
    duration = event["Duration"]
    parameters = event["Parameters"]
    event_intensity = parameters["Intensity"]
    event_frequency = parameters["Frequency"]
    if not isinstance(point, dict):
        if event_type == "continuous":
            analysis.compatibility_issue("invalid_curve_point", "Each curve point must be an object.")
        analysis.unreadable("Each Curve point must be an object.")
        return None
    time_ms = point.get("Time")
    intensity = point.get("Intensity")
    frequency = point.get("Frequency")
    if event_type == "continuous":
        _record_curve_point_issues(time_ms, intensity, frequency, duration, analysis)
        if not _is_curve_intensity(intensity):
            analysis.unreadable("Curve point intensity must be a finite number within 0.0..1.0.")
            return None
    if not _curve_point_scalars_are_readable(time_ms, intensity, frequency, analysis):
        return None
    return CurvePoint(
        time_ms=time_ms,
        intensity=int(round(event_intensity * float(intensity))),
        frequency=max(MIN_EVENT_FREQUENCY, min(MAX_EVENT_FREQUENCY, event_frequency + frequency)),
    )


def _curve_point_scalars_are_readable(
    time_ms: object,
    intensity: object,
    frequency: object,
    analysis: _PayloadAnalysis,
) -> bool:
    if (
        isinstance(time_ms, int)
        and isinstance(intensity, (int, float))
        and isinstance(frequency, int)
    ):
        return True
    analysis.unreadable("Curve point fields must use their declared scalar types.")
    return False


def _record_curve_point_issues(
    time_ms: object,
    intensity: object,
    frequency: object,
    duration: int,
    analysis: _PayloadAnalysis,
) -> None:
    if not isinstance(time_ms, int) or time_ms < 0 or time_ms > duration:
        analysis.compatibility_issue("invalid_curve_time", "Curve point time is out of range.")
    if not _is_curve_intensity(intensity):
        analysis.compatibility_issue(
            "invalid_curve_intensity",
            "Curve point intensity must be within 0.0..1.0.",
        )
    if not isinstance(frequency, int) or not MIN_CURVE_FREQUENCY <= frequency <= MAX_CURVE_FREQUENCY:
        analysis.compatibility_issue(
            "invalid_curve_frequency",
            "Curve point frequency is out of range.",
        )


def _is_curve_intensity(value: object) -> bool:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return False
    return math.isfinite(value) and MIN_CURVE_INTENSITY <= value <= MAX_CURVE_INTENSITY
