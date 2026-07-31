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

"""Profile loading tests for the DSP conversion policy boundary."""

from __future__ import annotations

import json
import tempfile
import unittest
from importlib.resources import files
from pathlib import Path

from audio2haptic.core.device_profile import DEFAULT_DEVICE_PROFILE
from audio2haptic.core.device_profile import MAX_PROFILE_FILE_BYTES
from audio2haptic.core.device_profile import MAX_SMOOTHING_KERNEL_POINTS
from audio2haptic.core.device_profile import ProfileError
from audio2haptic.core.device_profile import resolve_device_profile
from audio2haptic.core.models import HapticTone


def default_profile_data() -> dict:
    path = files("audio2haptic.resources").joinpath("haptic_profiles", "default.json")
    return json.loads(path.read_text(encoding="utf-8"))


def profile_error(data: object, name: str = "profile.json") -> str:
    with tempfile.TemporaryDirectory() as temporary_dir:
        path = Path(temporary_dir) / name
        if isinstance(data, str):
            path.write_text(data, encoding="utf-8")
        else:
            path.write_text(json.dumps(data), encoding="utf-8")
        try:
            resolve_device_profile(str(path))
        except ProfileError as exc:
            return str(exc)
    raise AssertionError("Expected profile loading to fail.")


class DeviceProfileTests(unittest.TestCase):
    def test_builtin_default_has_complete_style_and_policy_values(self) -> None:
        profile = DEFAULT_DEVICE_PROFILE

        self.assertEqual(profile.identity, "default@0.1.0")
        self.assertEqual(profile.policy.id, "default_v1")
        self.assertEqual(profile.frequency_for(HapticTone.TENSE), 70)
        self.assertIsInstance(profile.accent_relative_attenuation, int)
        self.assertIsInstance(profile.accent_relative_boost, int)
        self.assertIsInstance(profile.accent_min_intensity, int)
        self.assertIsInstance(profile.accent_max_intensity, int)

    def test_profile_file_read_errors_and_unknown_builtin_are_explicit(self) -> None:
        self.assertIn("Cannot read", profile_error("{", "invalid.json"))
        oversized = "x" * (MAX_PROFILE_FILE_BYTES + 1)
        self.assertIn("too large", profile_error(oversized, "oversized.json"))
        with self.assertRaises(ProfileError) as context:
            resolve_device_profile("does-not-exist")
        self.assertIn("Unknown haptic style profile", str(context.exception))

    def test_profile_rejects_invalid_shape_and_missing_required_values(self) -> None:
        invalid_kind = default_profile_data()
        invalid_kind["kind"] = "wrong"
        self.assertIn("Invalid style profile kind", profile_error(invalid_kind))

        invalid_parameters = default_profile_data()
        invalid_parameters["parameters"] = []
        self.assertIn("Incomplete style profile", profile_error(invalid_parameters))

        incomplete = default_profile_data()
        incomplete.pop("version")
        self.assertIn("Incomplete style profile", profile_error(incomplete))

    def test_profile_rejects_invalid_tone_and_intensity_groups(self) -> None:
        missing_tone = default_profile_data()
        missing_tone["parameters"]["tone_frequency"].pop("tense")
        self.assertIn("tone_frequency", profile_error(missing_tone))

        invalid_tone = default_profile_data()
        invalid_tone["parameters"]["tone_frequency"]["tense"] = True
        self.assertIn("tone_frequency", profile_error(invalid_tone))

        invalid_intensity = default_profile_data()
        invalid_intensity["parameters"]["texture_max_intensity"] = 101
        self.assertIn("intensity parameters", profile_error(invalid_intensity))

    def test_profile_rejects_invalid_ranges_kernel_and_policy(self) -> None:
        invalid_accent = default_profile_data()
        invalid_accent["parameters"]["accent_min_intensity"] = 90
        invalid_accent["parameters"]["accent_max_intensity"] = 50
        self.assertIn("accent intensity range", profile_error(invalid_accent))

        invalid_texture = default_profile_data()
        invalid_texture["parameters"]["texture_neutral_intensity"] = 0
        self.assertIn("texture intensity range", profile_error(invalid_texture))

        invalid_kernel = default_profile_data()
        invalid_kernel["parameters"]["texture_smoothing_kernel"] = [0.0] * (MAX_SMOOTHING_KERNEL_POINTS + 1)
        self.assertIn("texture_smoothing_kernel", profile_error(invalid_kernel))

        unknown_policy = default_profile_data()
        unknown_policy["dsp_policy"] = "unknown"
        self.assertIn("Unknown DSP policy", profile_error(unknown_policy))

    def test_external_profile_can_change_style_without_changing_policy(self) -> None:
        data = default_profile_data()
        data["id"] = "comfort"
        data["parameters"]["tone_frequency"]["tense"] = 62
        data["parameters"]["texture_max_intensity"] = 36
        with tempfile.TemporaryDirectory() as temporary_dir:
            path = Path(temporary_dir) / "comfort.json"
            path.write_text(json.dumps(data), encoding="utf-8")
            profile = resolve_device_profile(str(path))

        self.assertEqual(profile.frequency_for(HapticTone.TENSE), 62)
        self.assertEqual(profile.value("texture_max_intensity"), 36)
        self.assertEqual(profile.policy.id, DEFAULT_DEVICE_PROFILE.policy.id)


if __name__ == "__main__":
    unittest.main()
