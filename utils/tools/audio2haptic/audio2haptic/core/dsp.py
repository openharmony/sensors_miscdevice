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

from collections.abc import Iterator
from dataclasses import dataclass
import math

import numpy as np

from .device_profile import DEFAULT_DEVICE_PROFILE, DeviceProfile
from .models import AudioInput, CurvePoint, HapticEvent, HapticTone, StructuralRole
from .openharmony_contract import MAX_CONTINUOUS_DURATION_MS
from .openharmony_contract import MAX_CURVE_POINT_COUNT
from .openharmony_contract import MAX_EVENT_INTENSITY
from .openharmony_contract import MIN_CURVE_POINT_COUNT
from .openharmony_contract import TRANSIENT_DURATION_MS

MILLISECONDS_PER_SECOND = 1000
SPECTRAL_POWER_EPSILON = 1e-12
ONSET_FLUX_EPSILON = 1e-6
CURVE_VARIATION_EPSILON = 1e-5
RMS_EPSILON = 1e-8
AMPLITUDE_TO_DECIBEL_FACTOR = 20
MIN_RELATIVE_ACCENT_COUNT = 2


@dataclass(frozen=True)
class StructureMap:
    rms: np.ndarray
    low_band_ratio: np.ndarray
    spectral_flux: np.ndarray
    hop_size: int


@dataclass(frozen=True)
class _OnsetFeatures:
    previous_rms: np.ndarray
    difference: np.ndarray
    difference_threshold: float
    flux_threshold: float


@dataclass(frozen=True)
class _ContinuousChunk:
    start_ms: int
    duration_ms: int
    envelope: np.ndarray
    fade_in: bool
    fade_out: bool


def _frames(samples: np.ndarray, sample_rate: int, profile: DeviceProfile) -> tuple[np.ndarray, int, int]:
    frame_size = max(1, int(sample_rate * profile.policy.value("frame_ms") / MILLISECONDS_PER_SECOND))
    hop_size = max(1, int(sample_rate * profile.policy.value("hop_ms") / MILLISECONDS_PER_SECOND))
    starts = np.arange(0, max(samples.size, 1), hop_size)
    frames = np.zeros((len(starts), frame_size), dtype=np.float32)
    for index, start in enumerate(starts):
        frame = samples[start:start + frame_size]
        frames[index, slice(None, frame.size)] = frame
    return frames, frame_size, hop_size


def _structure_map(
    samples: np.ndarray,
    sample_rate: int,
    profile: DeviceProfile = DEFAULT_DEVICE_PROFILE,
) -> StructureMap:
    frames, frame_size, hop_size = _frames(samples, sample_rate, profile)
    rms = np.sqrt(np.mean(np.square(frames), axis=1))
    low_band_ratio, spectral_flux = _spectral_features(frames, frame_size, sample_rate, profile)
    return StructureMap(
        rms=rms,
        low_band_ratio=low_band_ratio,
        spectral_flux=spectral_flux,
        hop_size=hop_size,
    )


def _spectral_features(
    frames: np.ndarray,
    frame_size: int,
    sample_rate: int,
    profile: DeviceProfile,
) -> tuple[np.ndarray, np.ndarray]:
    window = np.hanning(frame_size).astype(np.float32)
    spectrum_power = np.square(np.abs(np.fft.rfft(frames * window[(None, slice(None))], axis=1)))
    freqs = np.fft.rfftfreq(frame_size, d=1.0 / sample_rate)
    low_mask = (freqs >= profile.policy.value("low_band_min_hz")) & (freqs <= profile.policy.value("low_band_max_hz"))
    low_power = np.sum(spectrum_power[:, low_mask], axis=1)
    total_power = np.sum(spectrum_power[:, 1:], axis=1)
    low_band_ratio = low_power / np.maximum(total_power, SPECTRAL_POWER_EPSILON)

    log_spectrum = np.log1p(spectrum_power)
    previous = np.vstack([log_spectrum[:1], log_spectrum[:-1]])
    spectral_flux = np.sum(np.maximum(log_spectrum - previous, 0.0), axis=1)
    spectral_flux[0] = 0.0
    return low_band_ratio, spectral_flux


def generate_candidate_events(
    audio: AudioInput,
    *,
    profile: DeviceProfile = DEFAULT_DEVICE_PROFILE,
) -> list[HapticEvent]:
    samples = np.asarray(audio.samples, dtype=np.float32)
    if samples.size == 0:
        return []
    samples = samples - np.median(samples)
    structure = _structure_map(samples, audio.sample_rate, profile)
    if float(structure.rms.max()) < profile.policy.value("silence_rms_floor"):
        return []
    accent_indices = _accent_indices(structure, profile)
    events = _accent_events(accent_indices, structure, audio.sample_rate, profile)
    events.extend(_bed_events(audio, structure, accent_indices, profile))
    return sorted(events, key=lambda event: (event.start_ms, 0 if event.kind == "continuous" else 1))


def _accent_events(
    accent_indices: list[int],
    structure: StructureMap,
    sample_rate: int,
    profile: DeviceProfile,
) -> list[HapticEvent]:
    intensities = _accent_intensities(accent_indices, structure, profile)
    return [
        _accent_event(index, structure, sample_rate, intensity=intensities[index], profile=profile)
        for index in accent_indices
    ]


def _bed_events(
    audio: AudioInput,
    structure: StructureMap,
    accent_indices: list[int],
    profile: DeviceProfile,
) -> list[HapticEvent]:
    events: list[HapticEvent] = []
    for start_index, end_index in _bed_segments(structure, accent_indices, profile):
        start_ms = _frame_start_ms(start_index, structure.hop_size, audio.sample_rate)
        end_ms = min(audio.duration_ms, _frame_start_ms(end_index, structure.hop_size, audio.sample_rate))
        duration_ms = max(0, end_ms - start_ms)
        if duration_ms >= profile.policy.value("bed_min_duration_ms"):
            events.extend(
                _continuous_events(
                    start_ms=start_ms,
                    duration_ms=duration_ms,
                    envelope=structure.rms[start_index:end_index],
                    profile=profile,
                )
            )
    return events


def _accent_indices(structure: StructureMap, profile: DeviceProfile) -> list[int]:
    policy = profile.policy
    candidates = _accent_candidates(structure, policy, _onset_features(structure, policy))
    return _spaced_accents(candidates, policy)


def _onset_features(structure: StructureMap, policy) -> _OnsetFeatures:
    previous_rms = np.concatenate(([0.0], structure.rms[:-1]))
    difference = structure.rms - previous_rms
    noise_delta = float(np.median(np.abs(difference))) if difference.size else 0.0
    difference_threshold = max(
        policy.value("onset_delta_floor"),
        noise_delta * policy.value("onset_noise_multiplier"),
    )
    flux_median = float(np.median(structure.spectral_flux))
    flux_mad = float(np.median(np.abs(structure.spectral_flux - flux_median)))
    return _OnsetFeatures(
        previous_rms=previous_rms,
        difference=difference,
        difference_threshold=difference_threshold,
        flux_threshold=flux_median + max(ONSET_FLUX_EPSILON, flux_mad * policy.value("onset_noise_multiplier")),
    )


def _accent_candidates(structure: StructureMap, policy, features: _OnsetFeatures) -> list[tuple[float, int]]:
    candidates: list[tuple[float, int]] = []
    for index, value in enumerate(features.difference):
        if not _admit_accent(index, value, structure, policy, features):
            continue
        rise_score = value / features.difference_threshold
        flux_score = float(structure.spectral_flux[index]) / max(features.flux_threshold, ONSET_FLUX_EPSILON)
        score = policy.value("onset_rise_weight") * rise_score + policy.value("onset_flux_weight") * flux_score
        candidates.append((score, index))
    return candidates


def _admit_accent(index: int, value: float, structure: StructureMap, policy, features: _OnsetFeatures) -> bool:
    rms = structure.rms
    relative_rise = value / max(float(features.previous_rms[index]), policy.value("silence_rms_floor"))
    is_local_peak = (index == 0 or value >= features.difference[index - 1]) and (
        index == len(features.difference) - 1 or value >= features.difference[index + 1]
    )
    if (
        value < features.difference_threshold
        or relative_rise < policy.value("onset_ratio_floor")
        or float(rms[index]) < policy.value("accent_admission_rms_floor")
        or not is_local_peak
    ):
        return False
    return index != 0 or len(rms) <= 1 or rms[0] > rms[1] * (1 + policy.value("onset_ratio_floor"))


def _spaced_accents(candidates: list[tuple[float, int]], policy) -> list[int]:
    selected: list[int] = []
    minimum_frame_gap = max(1, int(round(policy.value("accent_min_interval_ms") / policy.value("hop_ms"))))
    for _, index in sorted(candidates, reverse=True):
        if any(abs(index - kept) < minimum_frame_gap for kept in selected):
            continue
        selected.append(index)
    return sorted(selected)


def _accent_intensities(
    accent_indices: list[int], structure: StructureMap, profile: DeviceProfile
) -> dict[int, int]:
    """Keep absolute audibility as the gate, then widen contrast conservatively."""
    absolute = {index: _intensity_from_rms(float(structure.rms[index]), profile) for index in accent_indices}
    if len(accent_indices) < MIN_RELATIVE_ACCENT_COUNT:
        return absolute

    values = np.array([float(structure.rms[index]) for index in accent_indices], dtype=np.float32)
    low, high = np.percentile(
        values,
        (
            profile.policy.value("relative_intensity_low_percentile"),
            profile.policy.value("relative_intensity_high_percentile"),
        ),
    )
    if high - low < CURVE_VARIATION_EPSILON:
        return absolute

    intensities: dict[int, int] = {}
    for index, value in zip(accent_indices, values):
        relative_position = float(np.clip((value - low) / (high - low), 0.0, 1.0))
        adjustment = round(
            -profile.value("accent_relative_attenuation") * (1.0 - relative_position)
            + profile.value("accent_relative_boost") * relative_position
        )
        intensities[index] = int(
            np.clip(
                absolute[index] + adjustment,
                profile.value("accent_min_intensity"),
                profile.value("accent_max_intensity"),
            )
        )
    return intensities


def _accent_event(
    index: int,
    structure: StructureMap,
    sample_rate: int,
    *,
    intensity: int,
    profile: DeviceProfile,
) -> HapticEvent:
    tone = _accent_tone(index, intensity, structure, profile)
    return HapticEvent(
        start_ms=_frame_start_ms(index, structure.hop_size, sample_rate),
        kind="transient",
        intensity=intensity,
        frequency=profile.frequency_for(tone),
        source="dsp",
        duration_ms=TRANSIENT_DURATION_MS,
        tags=["accent", f"tone:{tone.value}"],
        structural_role=StructuralRole.ACCENT,
        haptic_tone=tone,
    )


def _accent_tone(index: int, intensity: int, structure: StructureMap, profile: DeviceProfile) -> HapticTone:
    policy = profile.policy
    if intensity <= policy.value("accent_soft_intensity"):
        return HapticTone.SOFT_ELASTIC
    flux_threshold = float(np.percentile(structure.spectral_flux, policy.value("accent_tense_flux_percentile")))
    if intensity >= policy.value("accent_tense_intensity") or float(structure.spectral_flux[index]) >= flux_threshold:
        return HapticTone.TENSE
    if float(structure.low_band_ratio[index]) >= policy.value("accent_heavy_low_band_ratio"):
        return HapticTone.HEAVY
    return HapticTone.NEUTRAL


def _bed_segments(structure: StructureMap, accent_indices: list[int], profile: DeviceProfile) -> list[tuple[int, int]]:
    policy = profile.policy
    rms = structure.rms
    peak_rms = float(np.percentile(rms, policy.value("bed_peak_percentile")))
    floor = max(policy.value("bed_absolute_rms_floor"), peak_rms * policy.value("bed_relative_rms_ratio"))
    baseline = float(np.percentile(rms, policy.value("bed_baseline_percentile")))
    baseline_ratio = baseline / max(peak_rms, policy.value("silence_rms_floor"))
    enough_accents = len(accent_indices) >= policy.value("transient_dominant_accent_count")
    if enough_accents and baseline_ratio < policy.value("bed_relative_rms_ratio"):
        floor = max(floor, peak_rms * policy.value("transient_dominant_bed_ratio"))

    active = (rms >= floor) & (structure.low_band_ratio >= policy.value("low_band_ratio_floor"))
    return _active_segments(active)


def _active_segments(active: np.ndarray) -> list[tuple[int, int]]:
    segments: list[tuple[int, int]] = []
    start_index: int | None = None
    for index, value in enumerate(active):
        if value and start_index is None:
            start_index = index
        elif not value and start_index is not None:
            segments.append((start_index, index))
            start_index = None
    if start_index is not None:
        segments.append((start_index, len(active)))
    return segments


def _continuous_events(
    start_ms: int,
    duration_ms: int,
    envelope: np.ndarray,
    *,
    profile: DeviceProfile,
) -> list[HapticEvent]:
    return [_continuous_event(chunk, profile) for chunk in _continuous_chunks(start_ms, duration_ms, envelope)]


def _continuous_chunks(
    start_ms: int,
    duration_ms: int,
    envelope: np.ndarray,
) -> Iterator[_ContinuousChunk]:
    total_frames = max(1, envelope.size)
    chunk_count = math.ceil(duration_ms / MAX_CONTINUOUS_DURATION_MS)
    base_duration, extra_milliseconds = divmod(duration_ms, chunk_count)
    current_start = start_ms
    for index in range(chunk_count):
        chunk_duration = base_duration + (index < extra_milliseconds)
        frame_start = round(total_frames * index / chunk_count)
        frame_end = round(total_frames * (index + 1) / chunk_count)
        chunk = envelope[frame_start:frame_end]
        yield _ContinuousChunk(
            start_ms=current_start,
            duration_ms=chunk_duration,
            envelope=chunk if chunk.size else envelope[-1:],
            fade_in=index == 0,
            fade_out=index == chunk_count - 1,
        )
        current_start += chunk_duration


def _continuous_event(chunk: _ContinuousChunk, profile: DeviceProfile) -> HapticEvent:
    control = _texture_control_track(chunk.envelope, profile)
    return HapticEvent(
        start_ms=chunk.start_ms,
        kind="continuous",
        duration_ms=chunk.duration_ms,
        intensity=round(float(np.mean(control))),
        frequency=profile.frequency_for(HapticTone.HEAVY),
        source="dsp",
        curve=_adaptive_texture_curve(
            control,
            chunk.duration_ms,
            fade_in=chunk.fade_in,
            fade_out=chunk.fade_out,
            profile=profile,
        ),
        tags=["bed", "tone:heavy"],
        structural_role=StructuralRole.BED,
        haptic_tone=HapticTone.HEAVY,
    )


def _texture_control_track(envelope: np.ndarray, profile: DeviceProfile) -> np.ndarray:
    values = np.asarray(envelope, dtype=np.float32)
    low = float(np.percentile(values, profile.policy.value("relative_intensity_low_percentile")))
    high = float(np.percentile(values, profile.policy.value("relative_intensity_high_percentile")))
    variation = high - low
    relative_variation = variation / max(abs(high), CURVE_VARIATION_EPSILON)
    minimum_variation = profile.policy.value("texture_min_relative_variation")
    if variation < CURVE_VARIATION_EPSILON or relative_variation < minimum_variation:
        return np.full(values.size, profile.value("texture_neutral_intensity"), dtype=np.float32)
    normalized = np.clip((values - low) / variation, 0.0, 1.0)
    minimum = profile.value("texture_min_intensity")
    control = minimum + (profile.value("texture_max_intensity") - minimum) * normalized
    kernel = np.asarray(profile.parameters["texture_smoothing_kernel"], dtype=np.float32)
    return np.convolve(control, kernel, mode="same")


def _adaptive_texture_curve(
    control: np.ndarray,
    duration_ms: int,
    *,
    fade_in: bool,
    fade_out: bool,
    profile: DeviceProfile,
) -> list[CurvePoint]:
    values = _curve_values(control, fade_in=fade_in, fade_out=fade_out)
    anchors = _curve_anchor_indices(values, float(profile.value("texture_curve_error")))
    return _curve_points(values, anchors, duration_ms, profile)


def _curve_values(control: np.ndarray, *, fade_in: bool, fade_out: bool) -> np.ndarray:
    values = np.asarray(control, dtype=np.float32).copy()
    if values.size == 0:
        values = np.zeros(1, dtype=np.float32)
    if fade_in:
        values[0] = 0
    if fade_out:
        values[-1] = 0
    return values


def _curve_anchor_indices(values: np.ndarray, threshold: float) -> set[int]:
    anchors = {0, len(values) - 1}
    while len(anchors) < MAX_CURVE_POINT_COUNT:
        candidate_index = _next_curve_anchor(values, anchors, threshold)
        if candidate_index is None:
            break
        anchors.add(candidate_index)
    if len(anchors) < MIN_CURVE_POINT_COUNT:
        anchors.update(int(index) for index in np.linspace(0, len(values) - 1, min(MIN_CURVE_POINT_COUNT, len(values))))
    return anchors


def _next_curve_anchor(values: np.ndarray, anchors: set[int], threshold: float) -> int | None:
    candidate_index = None
    candidate_error = threshold
    for left, right in zip(sorted(anchors), sorted(anchors)[1:]):
        if right - left <= 1:
            continue
        indices = np.arange(left + 1, right)
        interpolated = values[left] + (values[right] - values[left]) * (indices - left) / (right - left)
        errors = np.abs(values[indices] - interpolated)
        maximum = float(errors.max())
        if maximum > candidate_error:
            candidate_error = maximum
            candidate_index = int(indices[int(errors.argmax())])
    return candidate_index


def _curve_points(values: np.ndarray, anchors: set[int], duration_ms: int, profile: DeviceProfile) -> list[CurvePoint]:
    points = []
    indices = sorted(anchors)
    if len(indices) < MIN_CURVE_POINT_COUNT:
        indices = [round(index) for index in np.linspace(0, len(values) - 1, MIN_CURVE_POINT_COUNT)]
    for index in indices:
        time_ms = int(round(duration_ms * index / max(1, len(values) - 1)))
        points.append(
            CurvePoint(
                time_ms=time_ms,
                intensity=int(np.clip(round(values[index]), 0, MAX_EVENT_INTENSITY)),
                frequency=profile.frequency_for(HapticTone.HEAVY),
            )
        )
    return points


def _frame_start_ms(index: int, hop_size: int, sample_rate: int) -> int:
    return int(index * hop_size * MILLISECONDS_PER_SECOND / sample_rate)


def _intensity_from_rms(rms: float, profile: DeviceProfile = DEFAULT_DEVICE_PROFILE) -> int:
    level_db = AMPLITUDE_TO_DECIBEL_FACTOR * np.log10(max(rms, RMS_EPSILON))
    policy = profile.policy
    normalized = (level_db - policy.value("intensity_db_floor")) / policy.value("intensity_db_span")
    return int(
        max(
            policy.value("intensity_floor"),
            min(
                policy.value("intensity_ceiling"),
                round(
                    policy.value("intensity_floor")
                    + (policy.value("intensity_ceiling") - policy.value("intensity_floor")) * normalized
                ),
            ),
        )
    )
