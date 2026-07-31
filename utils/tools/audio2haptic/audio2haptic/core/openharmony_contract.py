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

"""Numeric limits implemented by the OpenHarmony haptic JSON decoder."""

FORMAT_VERSION = 1.0
CHANNEL_COUNT = 1
DEFAULT_CHANNEL_INDEX = 0
MAX_PATTERN_EVENT_COUNT = 128
DECODER_PACKET_EVENT_COUNT = 16
MAX_JSON_SIZE_BYTES = 64 * 1024
TRANSIENT_DURATION_MS = 48
MAX_CONTINUOUS_DURATION_MS = 5_000
MIN_CURVE_POINT_COUNT = 4
MAX_CURVE_POINT_COUNT = 16
MIN_EVENT_INTENSITY = 0
MAX_EVENT_INTENSITY = 100
MIN_EVENT_FREQUENCY = 0
MAX_EVENT_FREQUENCY = 100
MIN_CURVE_FREQUENCY = -100
MAX_CURVE_FREQUENCY = 100
MIN_CURVE_INTENSITY = 0.0
MAX_CURVE_INTENSITY = 1.0
MAX_EVENT_START_TIME_MS = 1_800_000
