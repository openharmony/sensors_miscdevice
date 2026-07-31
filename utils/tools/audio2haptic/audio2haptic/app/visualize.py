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

import html
import json
import math

from ..core.models import AudioInput
from ..core.openharmony_contract import MAX_EVENT_INTENSITY
from ..core.openharmony_contract import TRANSIENT_DURATION_MS

CHART_WIDTH_PX = 960
CHART_HEIGHT_PX = 160
WAVEFORM_POINT_COUNT = 96
MIN_EVENT_WIDTH_PX = 2
MIN_VISIBLE_EVENT_HEIGHT_PX = 8
SVG_COORDINATE_PRECISION = 2


def build_visualization_html(audio: AudioInput, payload: dict) -> str:
    waveform = _waveform_points(audio.samples)
    timeline = _timeline_svg(payload, max(audio.duration_ms, 1))
    metadata = payload.get("MetaData", {})
    event_count = len(payload.get("Channels", [{}])[0].get("Pattern", []))
    summary = _summary(audio, event_count, metadata.get("Version", "unknown"))
    return _document(summary, waveform, timeline)


def _waveform_points(samples: list[float]) -> str:
    if not samples:
        return ""
    samples_per_point = math.ceil(len(samples) / WAVEFORM_POINT_COUNT)
    values = [
        _window_amplitude(samples, start, samples_per_point)
        for start in range(0, len(samples), samples_per_point)
    ]
    maximum = max(values) or 1.0
    return " ".join(
        _waveform_coordinate(index, value, len(values), maximum)
        for index, value in enumerate(values)
    )


def _window_amplitude(samples: list[float], start: int, window_size: int) -> float:
    window = samples[slice(start, start + window_size)]
    return sum(abs(value) for value in window) / len(window)


def _waveform_coordinate(index: int, value: float, value_count: int, maximum: float) -> str:
    x = round(
        index * CHART_WIDTH_PX / max(value_count - 1, 1),
        SVG_COORDINATE_PRECISION,
    )
    y = round(
        CHART_HEIGHT_PX - value / maximum * CHART_HEIGHT_PX,
        SVG_COORDINATE_PRECISION,
    )
    return f"{x},{y}"


def _timeline_svg(payload: dict, duration_ms: int) -> str:
    events = payload.get("Channels", [{}])[0].get("Pattern", [])
    return "".join(_event_rect(item.get("Event", {}), duration_ms) for item in events)


def _event_rect(event: dict, duration_ms: int) -> str:
    start_ms = int(event.get("StartTime", 0))
    event_duration = _event_duration_ms(event)
    intensity = _event_intensity(event.get("Parameters", {}))
    x = _scale(start_ms, duration_ms, CHART_WIDTH_PX)
    width = max(MIN_EVENT_WIDTH_PX, _scale(event_duration, duration_ms, CHART_WIDTH_PX))
    height = _event_height_px(intensity)
    event_class = "continuous" if event.get("Type") == "continuous" else "transient"
    label = html.escape(json.dumps(event, ensure_ascii=True, separators=(",", ":")))
    return (
        f'<rect class="{event_class}" x="{x}" y="{CHART_HEIGHT_PX - height}" '
        f'width="{width}" height="{height}"><title>{label}</title></rect>'
    )


def _event_duration_ms(event: dict) -> int:
    if event.get("Type") == "continuous":
        return max(0, int(event.get("Duration", 0)))
    return TRANSIENT_DURATION_MS


def _event_intensity(parameters: dict) -> float:
    base_intensity = min(
        1.0,
        max(0.0, float(parameters.get("Intensity", 0)) / MAX_EVENT_INTENSITY),
    )
    curve = parameters.get("Curve")
    if isinstance(curve, list) and curve:
        values = [point.get("Intensity", 0.0) for point in curve if isinstance(point, dict)]
        return base_intensity * max((float(value) for value in values), default=0.0)
    return base_intensity


def _event_height_px(intensity: float) -> int:
    if intensity <= 0:
        return 0
    return max(MIN_VISIBLE_EVENT_HEIGHT_PX, round(intensity * CHART_HEIGHT_PX))


def _scale(value: int, total: int, width: int) -> int:
    return round(max(0, value) / max(total, 1) * width)


def _summary(audio: AudioInput, event_count: int, version: object) -> str:
    return (
        f"{html.escape(audio.path.name)} | {audio.input_format} | {audio.duration_ms} ms | "
        f"{event_count} events | JSON v{version}"
    )


def _document(summary: str, waveform: str, timeline: str) -> str:
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Audio2Haptic Visualization</title>
<style>
body {{ margin: 2rem auto; max-width: 960px; color: #172033; font-family: sans-serif; }}
svg {{ display: block; width: 100%; border: 1px solid #b9c3d3; background: #f8fafc; }}
section {{ margin-top: 1.5rem; }}
.audio {{ fill: none; stroke: #326ce5; stroke-width: 2; }}
.continuous {{ fill: #0057b8; }}
.transient {{ fill: #e36b19; }}
</style>
</head>
<body>
<h1>Audio2Haptic Visualization</h1>
<p>{summary}</p>
<section><h2>Audio waveform</h2>
<svg viewBox="0 0 {CHART_WIDTH_PX} {CHART_HEIGHT_PX}" role="img">
<polyline class="audio" points="{waveform}" /></svg></section>
<section><h2>Haptic timeline</h2>
<svg viewBox="0 0 {CHART_WIDTH_PX} {CHART_HEIGHT_PX}" role="img">{timeline}</svg></section>
</body>
</html>
"""
