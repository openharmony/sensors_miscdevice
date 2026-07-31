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

"""Decoder contract tests for the public OpenHarmony JSON payload."""
# pylint: disable=duplicate-code

from __future__ import annotations

import unittest

from audio2haptic.core.compatibility import CompatibilityError
from audio2haptic.core.compatibility import parse_openharmony_payload
from audio2haptic.core.compatibility import validate_openharmony_payload
from audio2haptic.core.models import CurvePoint, HapticEvent
from audio2haptic.core.renderer import render_openharmony


def valid_payload() -> dict:
    return {
        "MetaData": {"Version": 1.0, "ChannelNumber": 1},
        "Channels": [
            {
                "Parameters": {"Index": 0},
                "Pattern": [
                    {
                        "Event": {
                            "Type": "transient",
                            "StartTime": 0,
                            "Duration": 48,
                            "Parameters": {"Intensity": 60, "Frequency": 50},
                        }
                    }
                ],
            }
        ],
    }


def payload_with_curve() -> dict:
    payload = valid_payload()
    payload["Channels"][0]["Pattern"].append(
        {
            "Event": {
                "Type": "continuous",
                "StartTime": 48,
                "Duration": 200,
                "Parameters": {
                    "Intensity": 60,
                    "Frequency": 50,
                    "Curve": [
                        {"Time": 0, "Intensity": 0.0, "Frequency": 0},
                        {"Time": 50, "Intensity": 0.5, "Frequency": 0},
                        {"Time": 100, "Intensity": 1.0, "Frequency": 0},
                        {"Time": 200, "Intensity": 0.0, "Frequency": 0},
                    ],
                },
            }
        }
    )
    return payload


def decoder_error_code(payload: object) -> str:
    try:
        validate_openharmony_payload(payload)
    except CompatibilityError as exc:
        return exc.code
    raise AssertionError("Expected decoder validation to fail.")


def mutate(payload: dict, name: str) -> object:
    event = payload["Channels"][0]["Pattern"][0]["Event"]
    continuous = payload["Channels"][0]["Pattern"][1]["Event"]
    parameters = event["Parameters"]
    curve = continuous["Parameters"]["Curve"]
    mutations = {
        "invalid_payload": lambda: "not-an-object",
        "missing_metadata": lambda: payload.pop("MetaData"),
        "invalid_version": lambda: payload["MetaData"].__setitem__("Version", 2.0),
        "invalid_channel_number": lambda: payload["MetaData"].__setitem__("ChannelNumber", 2),
        "invalid_channels": lambda: payload.__setitem__("Channels", []),
        "invalid_channel": lambda: payload.__setitem__("Channels", [None]),
        "invalid_channel_parameters": lambda: payload["Channels"][0].__setitem__("Parameters", None),
        "invalid_channel_index": lambda: payload["Channels"][0]["Parameters"].__setitem__("Index", 1),
        "invalid_pattern": lambda: payload["Channels"][0].__setitem__("Pattern", "bad"),
        "too_many_events": lambda: payload["Channels"][0].__setitem__(
            "Pattern", payload["Channels"][0]["Pattern"] * 129
        ),
        "invalid_pattern_item": lambda: payload["Channels"][0].__setitem__("Pattern", [None]),
        "missing_event": lambda: payload["Channels"][0].__setitem__("Pattern", [{}]),
        "invalid_event_type": lambda: event.__setitem__("Type", "bad"),
        "invalid_start_time": lambda: event.__setitem__("StartTime", -1),
        "invalid_duration": lambda: continuous.__setitem__("Duration", "bad"),
        "invalid_continuous_duration": lambda: continuous.__setitem__("Duration", 5_001),
        "missing_parameters": lambda: event.__setitem__("Parameters", None),
        "invalid_intensity": lambda: parameters.__setitem__("Intensity", 101),
        "invalid_frequency": lambda: parameters.__setitem__("Frequency", 101),
        "invalid_curve_size": lambda: continuous["Parameters"].__setitem__("Curve", []),
        "invalid_curve_point": lambda: continuous["Parameters"].__setitem__("Curve", [None] * 4),
        "invalid_curve_time": lambda: curve[0].__setitem__("Time", 201),
        "invalid_curve_intensity": lambda: curve[0].__setitem__("Intensity", 2.0),
        "invalid_curve_frequency": lambda: curve[0].__setitem__("Frequency", 101),
    }
    mutation = mutations.get(name)
    if mutation is None:
        raise KeyError(name)
    candidate = mutation()
    return payload if candidate is None else candidate


class CompatibilityTests(unittest.TestCase):
    def test_valid_payload_and_curve_use_decoder_wire_ranges(self) -> None:
        validate_openharmony_payload(valid_payload())
        validate_openharmony_payload(payload_with_curve())

    def test_decoder_rejects_each_public_structure_boundary(self) -> None:
        for name in (
            "invalid_payload",
            "missing_metadata",
            "invalid_version",
            "invalid_channel_number",
            "invalid_channels",
            "invalid_channel",
            "invalid_channel_parameters",
            "invalid_channel_index",
            "invalid_pattern",
            "too_many_events",
            "invalid_pattern_item",
            "missing_event",
            "invalid_event_type",
            "invalid_start_time",
            "invalid_duration",
            "invalid_continuous_duration",
            "missing_parameters",
            "invalid_intensity",
            "invalid_frequency",
            "invalid_curve_size",
            "invalid_curve_point",
            "invalid_curve_time",
            "invalid_curve_intensity",
            "invalid_curve_frequency",
        ):
            with self.subTest(name=name):
                self.assertEqual(decoder_error_code(mutate(payload_with_curve(), name)), name)

    def test_transient_duration_is_ignored_by_the_decoder_contract(self) -> None:
        for duration in (None, 1, 48, 9_999):
            with self.subTest(duration=duration):
                payload = valid_payload()
                event = payload["Channels"][0]["Pattern"][0]["Event"]
                if duration is None:
                    event.pop("Duration")
                else:
                    event["Duration"] = duration
                parsed = parse_openharmony_payload(payload)
                self.assertEqual(parsed.require_events()[0].duration_ms, 48)

    def test_parser_keeps_readable_events_and_first_compatibility_error(self) -> None:
        payload = valid_payload()
        payload["Channels"][0]["Pattern"][0]["Event"]["Parameters"]["Frequency"] = 101

        parsed = parse_openharmony_payload(payload)

        self.assertEqual(parsed.compatibility_error.code, "invalid_frequency")
        self.assertEqual([(event.kind, event.frequency) for event in parsed.require_events()], [("transient", 101)])

    def test_parser_marks_unreadable_payload_without_masking_compatibility_error(self) -> None:
        payload = valid_payload()
        payload.pop("MetaData")
        payload["Channels"] = []

        parsed = parse_openharmony_payload(payload)

        self.assertEqual(parsed.compatibility_error.code, "missing_metadata")
        self.assertIsNone(parsed.events)
        self.assertEqual(decoder_error_code(payload), "missing_metadata")

    def test_json_size_and_non_finite_curve_intensity_are_rejected(self) -> None:
        payload = payload_with_curve()
        curve = payload["Channels"][0]["Pattern"][1]["Event"]["Parameters"]["Curve"]
        curve[0]["Intensity"] = float("nan")

        self.assertEqual(decoder_error_code(payload), "invalid_curve_intensity")

        with self.assertRaises(CompatibilityError) as context:
            validate_openharmony_payload(valid_payload(), json_size_bytes=65_537)
        self.assertEqual(context.exception.code, "json_too_large")

    def test_renderer_emits_relative_curve_values(self) -> None:
        events = [
            HapticEvent(
                start_ms=0,
                kind="continuous",
                intensity=60,
                source="test",
                frequency=50,
                duration_ms=200,
                curve=[
                    CurvePoint(time_ms=0, intensity=0, frequency=50),
                    CurvePoint(time_ms=50, intensity=40, frequency=50),
                    CurvePoint(time_ms=100, intensity=80, frequency=50),
                    CurvePoint(time_ms=200, intensity=0, frequency=50),
                ],
            )
        ]

        payload = render_openharmony(events)
        parameters = payload.get("Channels", [{}])[0].get("Pattern", [{}])[0].get("Event", {}).get("Parameters", {})
        values = parameters.get("Curve", [])

        self.assertEqual(parameters.get("Intensity"), 80)
        self.assertEqual([item.get("Intensity") for item in values], [0.0, 0.5, 1.0, 0.0])
        self.assertEqual([item.get("Frequency") for item in values], [0, 0, 0, 0])
        self.assertEqual(
            [point.intensity for point in parse_openharmony_payload(payload).require_events()[0].curve],
            [0, 40, 80, 0],
        )

    def test_renderer_keeps_zero_curve_intensity_finite(self) -> None:
        event = HapticEvent(
            start_ms=0,
            kind="continuous",
            intensity=0,
            source="test",
            frequency=50,
            duration_ms=200,
            curve=[CurvePoint(time_ms=index, intensity=0, frequency=50) for index in range(4)],
        )

        payload = render_openharmony([event])
        parameters = payload.get("Channels", [{}])[0].get("Pattern", [{}])[0].get("Event", {}).get("Parameters", {})

        self.assertEqual(parameters.get("Intensity"), 0)
        self.assertEqual(
            [point.get("Intensity") for point in parameters.get("Curve", [])],
            [0.0] * 4,
        )
        validate_openharmony_payload(payload)


if __name__ == "__main__":
    unittest.main()
