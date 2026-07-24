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

import unittest

import numpy as np

from tests.dsp_test_support import SAMPLE_RATE, audio_input
from audio2haptic.core.compatibility import validate_openharmony_payload
from audio2haptic.core.dsp import generate_candidate_events
from audio2haptic.core.renderer import render_openharmony


def _pulse(samples: np.ndarray, *, start_ms: int, amplitude: float, width_ms: int = 5) -> None:
    start = round((start_ms / 1000) * SAMPLE_RATE)
    width = round((width_ms / 1000) * SAMPLE_RATE)
    samples[start:start + width] = amplitude


def _transient_near(events, expected_ms: int) -> object | None:
    return next(
        (
            event
            for event in events
            if event.kind == "transient" and abs(event.start_ms - expected_ms) <= 40
        ),
        None,
    )


class DspPerceptualTest(unittest.TestCase):
    def test_silence_and_low_level_noise_do_not_trigger_haptics(self) -> None:
        silence = np.zeros(SAMPLE_RATE, dtype=np.float32)
        low_noise = np.random.default_rng(7).normal(0.0, 0.001, SAMPLE_RATE).astype(np.float32)
        broadband_noise = np.random.default_rng(5).normal(0.0, 0.05, SAMPLE_RATE).astype(np.float32)

        self.assertEqual(generate_candidate_events(audio_input(silence)), [])
        self.assertEqual(generate_candidate_events(audio_input(low_noise)), [])
        self.assertEqual(generate_candidate_events(audio_input(broadband_noise)), [])

    def test_boundary_impulses_are_detected_within_40ms(self) -> None:
        samples = np.zeros(SAMPLE_RATE, dtype=np.float32)
        _pulse(samples, start_ms=0, amplitude=0.9)
        _pulse(samples, start_ms=995, amplitude=0.9)

        events = generate_candidate_events(audio_input(samples))

        self.assertIsNotNone(_transient_near(events, 0))
        self.assertIsNotNone(_transient_near(events, 995))

    def test_long_low_frequency_signal_is_split_into_compatible_continuous_events(self) -> None:
        for duration_ms in (5001, 6000, 10_000):
            with self.subTest(duration_ms=duration_ms):
                sample_count = round((duration_ms / 1000) * SAMPLE_RATE)
                time = np.arange(sample_count, dtype=np.float32) / SAMPLE_RATE
                samples = 0.4 * np.sin(2 * np.pi * 100 * time)

                events = generate_candidate_events(audio_input(samples))
                continuous = [event for event in events if event.kind == "continuous"]

                self.assertTrue(continuous)
                self.assertTrue(all(event.duration_ms <= 5000 for event in continuous))
                self.assertGreaterEqual(sum(event.duration_ms for event in continuous), duration_ms - 40)
                for previous, current in zip(continuous, continuous[1:]):
                    self.assertLessEqual(current.start_ms - (previous.start_ms + previous.duration_ms), 20)
                validate_openharmony_payload(_render(events))

    def test_steady_low_frequency_texture_does_not_create_frame_phase_modulation(self) -> None:
        duration_ms = 4_000
        sample_count = round((duration_ms / 1000) * SAMPLE_RATE)
        time = np.arange(sample_count, dtype=np.float32) / SAMPLE_RATE
        samples = 0.24 * np.sin(2 * np.pi * 78 * time)

        events = generate_candidate_events(audio_input(samples))
        intensities = [point.intensity for event in events if event.kind == "continuous" for point in event.curve]

        self.assertGreaterEqual(len(intensities), 4)
        self.assertLessEqual(max(intensities[1:-1]) - min(intensities[1:-1]), 2)

    def test_stronger_onset_maps_to_higher_haptic_intensity(self) -> None:
        samples = np.zeros(SAMPLE_RATE, dtype=np.float32)
        _pulse(samples, start_ms=200, amplitude=0.35)
        _pulse(samples, start_ms=700, amplitude=0.9)

        events = generate_candidate_events(audio_input(samples))
        quiet = _transient_near(events, 200)
        strong = _transient_near(events, 700)

        self.assertIsNotNone(quiet)
        self.assertIsNotNone(strong)
        self.assertGreater(strong.intensity, quiet.intensity)


def _render(events) -> dict:
    return render_openharmony(events)


if __name__ == "__main__":
    unittest.main()
