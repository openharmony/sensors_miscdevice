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

from audio2haptic.core.models import HapticEvent
from audio2haptic.core.openharmony_contract import MAX_PATTERN_EVENT_COUNT
from audio2haptic.core.optimizer import optimize_events


def event(start_ms: int, kind: str, intensity: int = 50) -> HapticEvent:
    return HapticEvent(
        start_ms=start_ms,
        kind=kind,
        intensity=intensity,
        source="test",
        duration_ms=300 if kind == "continuous" else 48,
        frequency=50,
    )


class OptimizerTests(unittest.TestCase):
    def test_keeps_all_continuous_segments_when_global_capacity_remains(self) -> None:
        candidates = [event(index * 400, "continuous", 20 + index) for index in range(17)]
        candidates.append(event(7_500, "transient", 90))

        kept, dropped = optimize_events(candidates)

        self.assertEqual(len(kept), 18)
        self.assertEqual(sum(item.kind == "continuous" for item in kept), 17)
        self.assertEqual(sum(item.kind == "transient" for item in kept), 1)
        self.assertEqual(dropped, 0)

    def test_preserves_candidate_order_and_global_event_limit(self) -> None:
        candidates = [event(0, "transient"), event(100, "transient")]
        candidates.extend(
            event(200 + index * 130, "transient")
            for index in range(MAX_PATTERN_EVENT_COUNT + 1)
        )

        kept, dropped = optimize_events(candidates)

        self.assertEqual(len(kept), MAX_PATTERN_EVENT_COUNT)
        self.assertEqual([item.start_ms for item in kept], sorted(item.start_ms for item in kept))
        self.assertEqual(dropped, len(candidates) - len(kept))


if __name__ == "__main__":
    unittest.main()
