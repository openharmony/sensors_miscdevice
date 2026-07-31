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

import json
import math
from dataclasses import dataclass
from functools import lru_cache
from importlib.resources import files
from pathlib import Path

from .models import HapticTone
from .openharmony_contract import MAX_EVENT_INTENSITY
from .openharmony_contract import MIN_EVENT_INTENSITY


INTENSITY_PARAMETERS = (
    "accent_relative_attenuation",
    "accent_relative_boost",
    "accent_min_intensity",
    "accent_max_intensity",
    "texture_min_intensity",
    "texture_neutral_intensity",
    "texture_max_intensity",
    "texture_curve_error",
)
MAX_PROFILE_FILE_BYTES = 64 * 1024
MAX_SMOOTHING_KERNEL_POINTS = 64


class ProfileError(ValueError):
    pass


@dataclass(frozen=True)
class DspPolicy:
    id: str
    version: str
    parameters: dict[str, float | int]

    def value(self, name: str) -> float | int:
        return self.parameters[name]


@dataclass(frozen=True)
class DeviceProfile:
    name: str
    version: str
    policy: DspPolicy
    parameters: dict[str, object]

    @property
    def identity(self) -> str:
        return f"{self.name}@{self.version}"

    @property
    def accent_relative_attenuation(self) -> int:
        return int(self.value("accent_relative_attenuation"))

    @property
    def accent_relative_boost(self) -> int:
        return int(self.value("accent_relative_boost"))

    @property
    def accent_min_intensity(self) -> int:
        return int(self.value("accent_min_intensity"))

    @property
    def accent_max_intensity(self) -> int:
        return int(self.value("accent_max_intensity"))

    def value(self, name: str) -> float | int:
        return self.parameters[name]  # type: ignore[return-value]

    def frequency_for(self, tone: HapticTone) -> int:
        return int(self.parameters["tone_frequency"][tone.value])  # type: ignore[index]


def _read_json(path: Path) -> dict:
    try:
        if path.stat().st_size > MAX_PROFILE_FILE_BYTES:
            raise ProfileError(f"Haptic profile configuration is too large: {path}")
        return json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise ProfileError(f"Cannot read haptic profile configuration {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise ProfileError(f"Cannot read haptic profile configuration {path}: invalid JSON") from exc


@lru_cache
def _dsp_policy(name: str) -> DspPolicy:
    path = files("audio2haptic.resources").joinpath("haptic_policies", f"{name}.json")
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise ProfileError(f"Unknown DSP policy {name}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise ProfileError(f"DSP policy is invalid JSON: {name}") from exc
    if data.get("kind") != "audio2haptic_dsp_policy":
        raise ProfileError(f"Invalid DSP policy: {name}")
    try:
        return DspPolicy(id=data["id"], version=data["version"], parameters=data["parameters"])
    except (KeyError, TypeError) as exc:
        raise ProfileError(f"Incomplete DSP policy: {name}") from exc


def _valid_profile_intensity(value: object) -> bool:
    return (
        isinstance(value, int)
        and not isinstance(value, bool)
        and MIN_EVENT_INTENSITY <= value <= MAX_EVENT_INTENSITY
    )


def _profile_parameters(data: dict) -> dict:
    parameters = data["parameters"]
    if not isinstance(parameters, dict):
        raise ProfileError("Incomplete style profile.")
    return parameters


def _validate_tone_frequency(parameters: dict) -> None:
    tone = parameters["tone_frequency"]
    if not isinstance(tone, dict) or set(tone) != {item.value for item in HapticTone}:
        raise ProfileError("Style profile tone_frequency is incomplete.")
    if any(not _valid_profile_intensity(value) for value in tone.values()):
        raise ProfileError("Style profile tone_frequency is invalid.")


def _validate_intensity_parameters(parameters: dict) -> None:
    if any(name not in parameters for name in INTENSITY_PARAMETERS):
        raise ProfileError("Incomplete style profile.")
    if any(not _valid_profile_intensity(parameters[name]) for name in INTENSITY_PARAMETERS):
        raise ProfileError("Style profile intensity parameters are invalid.")
    if parameters["accent_min_intensity"] > parameters["accent_max_intensity"]:
        raise ProfileError("Style profile accent intensity range is invalid.")
    if not (
        parameters["texture_min_intensity"]
        <= parameters["texture_neutral_intensity"]
        <= parameters["texture_max_intensity"]
    ):
        raise ProfileError("Style profile texture intensity range is invalid.")


def _validate_smoothing_kernel(parameters: dict) -> None:
    kernel = parameters["texture_smoothing_kernel"]
    if not isinstance(kernel, list) or not kernel or len(kernel) > MAX_SMOOTHING_KERNEL_POINTS:
        raise ProfileError("Style profile texture_smoothing_kernel is invalid.")
    invalid_value = any(
        not isinstance(value, (int, float))
        or isinstance(value, bool)
        or not math.isfinite(value)
        or value < 0
        for value in kernel
    )
    if invalid_value or sum(kernel) <= 0:
        raise ProfileError("Style profile texture_smoothing_kernel is invalid.")


def _profile_from_data(data: dict) -> DeviceProfile:
    if not isinstance(data, dict) or data.get("kind") != "audio2haptic_style_profile":
        raise ProfileError("Invalid style profile kind.")
    try:
        parameters = _profile_parameters(data)
        _validate_tone_frequency(parameters)
        _validate_intensity_parameters(parameters)
        _validate_smoothing_kernel(parameters)
        return DeviceProfile(
            name=data["id"],
            version=data["version"],
            policy=_dsp_policy(data["dsp_policy"]),
            parameters=parameters,
        )
    except (KeyError, TypeError) as exc:
        raise ProfileError("Incomplete style profile.") from exc


def resolve_device_profile(identifier: str) -> DeviceProfile:
    path = Path(identifier)
    if path.is_file():
        return _profile_from_data(_read_json(path))
    return _builtin_device_profile(identifier)


@lru_cache
def _builtin_device_profile(identifier: str) -> DeviceProfile:
    resource = files("audio2haptic.resources").joinpath("haptic_profiles", f"{identifier}.json")
    try:
        return _profile_from_data(json.loads(resource.read_text(encoding="utf-8")))
    except FileNotFoundError as exc:
        raise ProfileError(f"Unknown haptic style profile: {identifier}") from exc
    except OSError as exc:
        raise ProfileError(f"Cannot read haptic style profile {identifier}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise ProfileError(f"Haptic style profile is invalid JSON: {identifier}") from exc


DEFAULT_DEVICE_PROFILE = resolve_device_profile("default")
