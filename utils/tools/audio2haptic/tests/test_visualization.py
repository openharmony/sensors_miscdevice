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

"""Self-contained HTML visualization tests."""
# pylint: disable=duplicate-code

from __future__ import annotations

import unittest
from pathlib import Path

from audio2haptic.app.visualize import build_visualization_html
from audio2haptic.app.visualize import _waveform_points
from audio2haptic.core.models import AudioInput


def visualization_audio(samples: list[float], duration_ms: int) -> AudioInput:
    return AudioInput(
        path=Path("<audio>.wav"),
        input_format="wav",
        duration_ms=duration_ms,
        sample_rate=1_000,
        input_channels=1,
        channel_policy="mono_passthrough",
        samples=samples,
        decode_backend="test",
    )


def visualization_payload() -> dict:
    return {
        "MetaData": {"Version": 1.0},
        "Channels": [
            {
                "Pattern": [
                    {
                        "Event": {
                            "Type": "transient",
                            "StartTime": 0,
                            "Duration": 48,
                            "Parameters": {"Intensity": 50, "Frequency": 50},
                        }
                    },
                    {
                        "Event": {
                            "Type": "continuous",
                            "StartTime": 48,
                            "Duration": 200,
                            "Parameters": {
                                "Intensity": 20,
                                "Frequency": 50,
                                "Curve": [
                                    {"Time": 0, "Intensity": 0.0, "Frequency": 0},
                                    {"Time": 100, "Intensity": 0.8, "Frequency": 0},
                                ],
                            },
                        }
                    },
                ]
            }
        ],
    }


class VisualizationTests(unittest.TestCase):
    def test_html_escapes_audio_name_and_renders_audio_and_haptic_views(self) -> None:
        html = build_visualization_html(visualization_audio([0.0, 0.5, -1.0], 248), visualization_payload())

        self.assertIn("&lt;audio&gt;.wav", html)
        self.assertIn("Audio waveform", html)
        self.assertIn("Haptic timeline", html)
        self.assertIn('class="transient"', html)
        self.assertIn('class="continuous"', html)
        self.assertIn('height="26"', html)

    def test_empty_audio_with_zero_duration_still_renders_valid_static_document(self) -> None:
        html = build_visualization_html(visualization_audio([], 0), visualization_payload())

        self.assertIn('points=""', html)
        self.assertIn("0 ms", html)
        self.assertIn("2 events", html)
        self.assertNotIn("http://", html)
        self.assertNotIn("https://", html)

    def test_waveform_downsampling_includes_the_final_input_sample(self) -> None:
        points = _waveform_points([0.0] * 99 + [1.0])

        self.assertTrue(points.endswith("960.0,0.0"), points)

    def test_zero_intensity_event_has_no_false_visible_amplitude(self) -> None:
        payload = visualization_payload()
        event = payload.get("Channels", [{}])[0].get("Pattern", [{}])[0].get("Event", {})
        event.get("Parameters", {}).update({"Intensity": 0})

        html = build_visualization_html(visualization_audio([0.0], 248), payload)

        self.assertIn('class="transient" x="0" y="160"', html)
        self.assertIn('height="0"', html)


if __name__ == "__main__":
    unittest.main()
