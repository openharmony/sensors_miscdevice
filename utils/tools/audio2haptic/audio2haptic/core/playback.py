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

"""Make event plans safe for the decoder's fixed-size playback packets."""

from __future__ import annotations

from dataclasses import replace

from .models import CurvePoint
from .models import HapticEvent
from .openharmony_contract import DECODER_PACKET_EVENT_COUNT
from .openharmony_contract import MAX_CURVE_POINT_COUNT
from .openharmony_contract import MIN_CURVE_POINT_COUNT


def plan_packet_safe_events(events: list[HapticEvent], max_events: int) -> tuple[list[HapticEvent], int]:
    """Split continuous events at every packet handoff without exceeding capacity."""
    retained = _ordered(events)
    dropped = 0
    while True:
        unsplittable = _unplayable_packet_event(retained) or _unsplittable_continuous(retained)
        if unsplittable is not None:
            retained.remove(unsplittable)
            dropped += 1
            continue
        planned = _split_packet_crossings(retained)
        if len(planned) <= max_events:
            return planned, dropped
        event = _drop_for_capacity(retained)
        if event is None:
            raise RuntimeError("Packet planner cannot satisfy the event capacity.")
        retained.remove(event)
        dropped += 1


def packet_crossings(events: list[HapticEvent]) -> list[tuple[HapticEvent, int]]:
    """Return continuous events that would be interrupted by a packet handoff."""
    ordered = _ordered(events)
    crossings: list[tuple[HapticEvent, int]] = []
    for group_end in range(DECODER_PACKET_EVENT_COUNT, len(ordered), DECODER_PACKET_EVENT_COUNT):
        handoff_ms = ordered[group_end].start_ms
        for event in ordered[slice(group_end - DECODER_PACKET_EVENT_COUNT, group_end)]:
            if event.kind == "continuous" and _event_end_ms(event) > handoff_ms:
                crossings.append((event, handoff_ms))
    return crossings


def _split_packet_crossings(events: list[HapticEvent]) -> list[HapticEvent]:
    planned = list(events)
    crossings = packet_crossings(planned)
    while crossings:
        event, handoff_ms = crossings[0]
        planned = _replace_event(planned, event, _split_continuous_event(event, handoff_ms))
        crossings = packet_crossings(planned)
    return _ordered(planned)


def _split_continuous_event(event: HapticEvent, handoff_ms: int) -> list[HapticEvent]:
    end_ms = _event_end_ms(event)
    return [
        replace(
            event,
            duration_ms=handoff_ms - event.start_ms,
            curve=_curve_slice(event, event.start_ms, handoff_ms),
        ),
        replace(
            event,
            start_ms=handoff_ms,
            duration_ms=end_ms - handoff_ms,
            curve=_curve_slice(event, handoff_ms, end_ms),
        ),
    ]


def _curve_slice(event: HapticEvent, start_ms: int, end_ms: int) -> list[CurvePoint]:
    if not event.curve:
        return []
    offset_start = start_ms - event.start_ms
    offset_end = end_ms - event.start_ms
    points = sorted(event.curve, key=lambda point: point.time_ms)
    times = _curve_sample_times(points, offset_start, offset_end)
    return [
        CurvePoint(
            time_ms=time_ms - offset_start,
            intensity=_interpolate_intensity(points, time_ms, event.intensity),
            frequency=_interpolate_frequency(points, time_ms, event.frequency),
        )
        for time_ms in times
    ]


def _curve_sample_times(points: list[CurvePoint], start_ms: int, end_ms: int) -> list[int]:
    times = [start_ms]
    times.extend(point.time_ms for point in points if start_ms < point.time_ms < end_ms)
    times.append(end_ms)
    unique = sorted(set(times))
    interpolation_segments = MIN_CURVE_POINT_COUNT - 1
    for numerator in range(1, interpolation_segments):
        candidate = round(start_ms + (end_ms - start_ms) * numerator / interpolation_segments)
        if candidate not in unique:
            unique.append(candidate)
            unique.sort()
        if len(unique) >= MIN_CURVE_POINT_COUNT:
            break
    while len(unique) < MIN_CURVE_POINT_COUNT:
        unique.insert(1, start_ms)
    if len(unique) <= MAX_CURVE_POINT_COUNT:
        return unique
    last_index = MAX_CURVE_POINT_COUNT - 1
    return [unique[round(index * (len(unique) - 1) / last_index)] for index in range(MAX_CURVE_POINT_COUNT)]


def _interpolate_intensity(points: list[CurvePoint], time_ms: int, fallback: int) -> int:
    left, right = _curve_neighbors(points, time_ms)
    if left is None:
        return fallback
    if right is None or right.time_ms == left.time_ms:
        return left.intensity
    ratio = (time_ms - left.time_ms) / (right.time_ms - left.time_ms)
    return round(left.intensity + (right.intensity - left.intensity) * ratio)


def _interpolate_frequency(points: list[CurvePoint], time_ms: int, fallback: int | None) -> int | None:
    left, right = _curve_neighbors(points, time_ms)
    left_frequency = fallback if left is None or left.frequency is None else left.frequency
    right_frequency = left_frequency if right is None or right.frequency is None else right.frequency
    if left_frequency is None or right is None or right.time_ms == left.time_ms:
        return left_frequency
    ratio = (time_ms - left.time_ms) / (right.time_ms - left.time_ms)
    return round(left_frequency + (right_frequency - left_frequency) * ratio)


def _curve_neighbors(points: list[CurvePoint], time_ms: int) -> tuple[CurvePoint | None, CurvePoint | None]:
    left = None
    for point in points:
        if point.time_ms >= time_ms:
            return left or point, point
        left = point
    return left, None


def _replace_event(
    events: list[HapticEvent],
    original: HapticEvent,
    replacement: list[HapticEvent],
) -> list[HapticEvent]:
    for index, event in enumerate(events):
        if event is original:
            before = events[slice(None, index)]
            after = events[slice(index + 1, None)]
            return before + replacement + after
    raise RuntimeError("Packet planner lost a continuous event.")


def _unsplittable_continuous(events: list[HapticEvent]) -> HapticEvent | None:
    for event, handoff_ms in packet_crossings(events):
        if handoff_ms <= event.start_ms:
            return event
    return None


def _unplayable_packet_event(events: list[HapticEvent]) -> HapticEvent | None:
    ordered = _ordered(events)
    for group_end in range(DECODER_PACKET_EVENT_COUNT, len(ordered), DECODER_PACKET_EVENT_COUNT):
        group_start = ordered[group_end - DECODER_PACKET_EVENT_COUNT].start_ms
        if ordered[group_end].start_ms == group_start:
            return ordered[group_end]
    return None


def _drop_for_capacity(events: list[HapticEvent]) -> HapticEvent | None:
    transient = [event for event in events if event.kind == "transient"]
    if transient:
        return max(transient, key=lambda event: event.start_ms)
    continuous = [event for event in events if event.kind == "continuous"]
    if continuous:
        return min(continuous, key=lambda event: (event.intensity, event.start_ms))
    return None


def _event_end_ms(event: HapticEvent) -> int:
    return event.start_ms + (event.duration_ms or 0)


def _ordered(events: list[HapticEvent]) -> list[HapticEvent]:
    return sorted(events, key=lambda event: (event.start_ms, event.kind != "transient"))
