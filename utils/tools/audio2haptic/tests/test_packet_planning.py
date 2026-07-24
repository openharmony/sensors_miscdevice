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

"""Regression tests for decoder packet boundaries."""

from __future__ import annotations

import unittest

from audio2haptic.core.compatibility import validate_openharmony_payload
from audio2haptic.core.models import CurvePoint
from audio2haptic.core.models import HapticEvent
from audio2haptic.core.optimizer import optimize_events
from audio2haptic.core.playback import plan_packet_safe_events
from audio2haptic.core.renderer import render_openharmony


def transient(start_ms: int) -> HapticEvent:
    return HapticEvent(start_ms, "transient", 75, "test", duration_ms=48, frequency=48)


def continuous(duration_ms: int = 5_000) -> HapticEvent:
    return HapticEvent(
        0,
        "continuous",
        64,
        "test",
        duration_ms=duration_ms,
        frequency=48,
        curve=[
            CurvePoint(0, 64, 48),
            CurvePoint(duration_ms // 3, 50, 48),
            CurvePoint(duration_ms * 2 // 3, 72, 48),
            CurvePoint(duration_ms, 64, 48),
        ],
    )


def decoder_packet_crossings(events: list[HapticEvent]) -> list[tuple[int, int]]:
    ordered = sorted(events, key=lambda event: event.start_ms)
    crossings = []
    for group_end in range(16, len(ordered), 16):
        handoff_ms = ordered[group_end].start_ms
        for event in ordered[slice(group_end - 16, group_end)]:
            event_end_ms = event.start_ms + (event.duration_ms or 0)
            if event.kind == "continuous" and event_end_ms > handoff_ms:
                crossings.append((event.start_ms, handoff_ms))
    return crossings


def decoder_packet_plan_is_playable(events: list[HapticEvent]) -> bool:
    ordered = sorted(events, key=lambda event: event.start_ms)
    for group_end in range(16, len(ordered), 16):
        if ordered[group_end].start_ms == ordered[group_end - 16].start_ms:
            return False
    return not decoder_packet_crossings(events)


class PacketPlanningTests(unittest.TestCase):
    def test_continuous_event_is_split_before_the_decoder_handoff(self) -> None:
        candidate_events = [continuous()]
        candidate_events.extend(transient(100 + index * 120) for index in range(15))
        candidate_events.append(transient(2_000))

        events, dropped = optimize_events(candidate_events)
        continuous_events = [event for event in events if event.kind == "continuous"]
        payload = render_openharmony(events)

        self.assertEqual(dropped, 0)
        self.assertEqual(
            [(event.start_ms, event.duration_ms) for event in continuous_events],
            [(0, 2_000), (2_000, 3_000)],
        )
        self.assertTrue(decoder_packet_plan_is_playable(events))
        validate_openharmony_payload(payload)

    def test_simultaneous_handoff_drops_an_unplayable_event(self) -> None:
        candidate_events = [transient(0) for _ in range(15)]
        candidate_events.extend([continuous(), continuous()])

        events, dropped = plan_packet_safe_events(candidate_events, max_events=128)

        self.assertEqual(dropped, 1)
        self.assertEqual(len(events), 16)
        self.assertTrue(decoder_packet_plan_is_playable(events))

    def test_same_time_events_do_not_create_multiple_playback_groups(self) -> None:
        candidate_events = [transient(0) for _ in range(16)] + [continuous()]

        events, dropped = plan_packet_safe_events(candidate_events, max_events=128)

        self.assertEqual(dropped, 1)
        self.assertEqual(len(events), 16)
        self.assertTrue(decoder_packet_plan_is_playable(events))

    def test_multiple_handoffs_and_capacity_pressure_stay_decoder_safe(self) -> None:
        accent_events = [transient(100 + index * 120) for index in range(127)]

        events, dropped = plan_packet_safe_events([continuous(10_000)] + accent_events, max_events=128)
        payload = render_openharmony(events)
        rendered_events = [item["Event"] for item in payload["Channels"][0]["Pattern"]]

        self.assertLessEqual(len(events), 128)
        self.assertGreater(dropped, 0)
        self.assertTrue(decoder_packet_plan_is_playable(events))
        continuous_events = (event for event in rendered_events if event["Type"] == "continuous")
        self.assertTrue(all(4 <= len(event["Parameters"]["Curve"]) <= 16 for event in continuous_events))
        validate_openharmony_payload(payload)


if __name__ == "__main__":
    unittest.main()
