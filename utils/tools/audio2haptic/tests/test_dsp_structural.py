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
import unittest

import numpy as np

from tests.dsp_test_support import SAMPLE_RATE, add_decaying_accent, audio_input
from audio2haptic.core.dsp import generate_candidate_events
from audio2haptic.core.models import StructuralRole


class StructuralDspTests(unittest.TestCase):
    def test_transient_dominant_knock_discards_weak_low_frequency_background(self) -> None:
        duration_ms = 2_400
        times = np.arange(round(duration_ms * SAMPLE_RATE / 1000), dtype=np.float32) / SAMPLE_RATE
        samples = 0.025 * np.sin(2 * math.pi * 42 * times)
        onsets = (160, 520, 940, 1_420, 1_900)
        for onset in onsets:
            add_decaying_accent(samples, onset, 0.72, clip_to_buffer=True)

        events = generate_candidate_events(audio_input(samples))
        transients = [event for event in events if event.kind == "transient"]
        continuous = [event for event in events if event.kind == "continuous"]

        self.assertFalse(continuous, "weak knock background must not become a vibration bed")
        self.assertTrue(all(event.duration_ms == 48 for event in transients))
        self.assertTrue(all(event.structural_role is StructuralRole.ACCENT for event in transients))
        for onset in onsets:
            self.assertTrue(any(abs(event.start_ms - onset) <= 40 for event in transients), onset)

    def test_sustained_low_frequency_rumble_keeps_a_modulated_bed(self) -> None:
        duration_ms = 3_000
        times = np.arange(round(duration_ms * SAMPLE_RATE / 1000), dtype=np.float32) / SAMPLE_RATE
        envelope = 0.16 + 0.14 * (0.5 + 0.5 * np.sin(2 * math.pi * 0.7 * times))
        samples = envelope * np.sin(2 * math.pi * 78 * times)

        events = generate_candidate_events(audio_input(samples))
        continuous = [event for event in events if event.kind == "continuous"]

        self.assertTrue(continuous, "sustained low-frequency content needs a vibration bed")
        self.assertTrue(all(event.structural_role is StructuralRole.BED for event in continuous))
        self.assertGreaterEqual(sum(event.duration_ms or 0 for event in continuous), 2_500)
        curve_intensities = [point.intensity for event in continuous for point in event.curve]
        self.assertGreaterEqual(max(curve_intensities) - min(curve_intensities), 20)

    def test_strong_accent_over_sustained_rumble_keeps_both_event_types(self) -> None:
        duration_ms = 3_000
        times = np.arange(round(duration_ms * SAMPLE_RATE / 1000), dtype=np.float32) / SAMPLE_RATE
        envelope = 0.14 + 0.08 * (0.5 + 0.5 * np.sin(2 * math.pi * 0.6 * times))
        samples = envelope * np.sin(2 * math.pi * 78 * times)
        add_decaying_accent(samples, 1_200, 0.75, clip_to_buffer=True)

        events = generate_candidate_events(audio_input(samples))
        continuous = [event for event in events if event.kind == "continuous"]
        transients = [event for event in events if event.kind == "transient"]

        self.assertTrue(continuous, "the low-frequency bed must survive a foreground accent")
        self.assertTrue(any(abs(event.start_ms - 1_200) <= 40 for event in transients))

    def test_weak_stationary_low_frequency_hum_is_not_a_bed(self) -> None:
        duration_ms = 3_000
        times = np.arange(round(duration_ms * SAMPLE_RATE / 1000), dtype=np.float32) / SAMPLE_RATE
        samples = 0.025 * np.sin(2 * math.pi * 78 * times)

        events = generate_candidate_events(audio_input(samples))
        self.assertEqual(events, [])

    def test_continuous_bed_never_extends_past_non_hop_aligned_input(self) -> None:
        duration_ms = 3_001
        times = np.arange(round(duration_ms * SAMPLE_RATE / 1000), dtype=np.float32) / SAMPLE_RATE
        samples = 0.25 * np.sin(2 * math.pi * 78 * times)

        continuous = [event for event in generate_candidate_events(audio_input(samples)) if event.kind == "continuous"]
        self.assertTrue(continuous)
        self.assertLessEqual(max(event.start_ms + (event.duration_ms or 0) for event in continuous), duration_ms)

    def test_accent_strength_maps_to_ordered_transient_intensity(self) -> None:
        samples = np.zeros(round(1_400 * SAMPLE_RATE / 1000), dtype=np.float32)
        add_decaying_accent(samples, 220, 0.22, clip_to_buffer=True)
        add_decaying_accent(samples, 820, 0.76, clip_to_buffer=True)

        transients = [event for event in generate_candidate_events(audio_input(samples)) if event.kind == "transient"]
        weak = min(transients, key=lambda event: abs(event.start_ms - 220))
        strong = min(transients, key=lambda event: abs(event.start_ms - 820))
        self.assertGreater(strong.intensity, weak.intensity)


if __name__ == "__main__":
    unittest.main()
