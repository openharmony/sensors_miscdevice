/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cmath>
#include <map>

#include "custom_vibration_matcher.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
namespace {
std::map<int32_t, std::vector<int32_t>> TRANSIENT_VIBRATION_INFOS = {
    {40, {77, 77, 11}}, {44, {42, 100, 7}}, {48, {68, 82, 22}},
    {60, {69, 52, 10}}, {64, {46, 67, 10}}, {72, {81, 82, 10}},
    {76, {58, 12, 15}}, {80, {100, 32, 20}}, {84, {85, 52, 28}},
    {92, {50, 12, 19}}, {96, {18, 7, 10}}
};  // {Num, {intensity, frequency, duration}}
constexpr int32_t INTENSITY_MIN = 0;
constexpr int32_t INTENSITY_MAX = 100;
constexpr int32_t MIN_TIME_SPACE = 20;
constexpr int32_t TRANSIENT_GRADE_NUM = 4;
constexpr int32_t CONTINUOUS_GRADE_NUM = 8;
constexpr float INTENSITY_WEIGHT = 0.5;
constexpr float FREQUENCY_WEIGHT = 0.5;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "CustomVibrationMatcher" };
}  // namespace

CustomVibrationMatcher::CustomVibrationMatcher(const std::vector<VibrateEvent>& vibrateSequence)
{
    MISC_HILOGD("enter CustomVibrationMatcher");
    for (const auto& event : vibrateSequence) {
        if (event.tag == EVENT_TAG_CONTINUOUS) {
            ProcessContinuousEvent(event);
        } else if (event.tag == EVENT_TAG_TRANSIENT) {
            ProcessTransientEvent(event);
        } else {
            MISC_HILOGE("The type of event is invalid : %{public}d", event.tag);
            return;
        }
    }
}

void CustomVibrationMatcher::ProcessContinuousEvent(const VibrateEvent& event)
{
    float scale = (INTENSITY_MAX - INTENSITY_MIN) / CONTINUOUS_GRADE_NUM;
    int32_t grade = -1;
    float minDistance = INTENSITY_MAX - INTENSITY_MIN;
    for (size_t i = 1; i <= 8; ++i) {
        if (std::abs(event.intensity - i * scale) < minDistance) {
            grade = i - 1;
            minDistance = std::abs(event.intensity - i * scale);
        }
    }
    if (grade == -1) {
        MISC_HILOGE("Continuous event match failed");
        return;
    }
    int32_t matchId = event.duration * 100 + grade;
    convertSequence_.push_back(event.delayTime);
    convertSequence_.push_back(matchId);
}

void CustomVibrationMatcher::ProcessTransientEvent(const VibrateEvent& event)
{
    std::vector<std::vector<float>> distanceInfo;
    for (const auto& transientInfo : TRANSIENT_VIBRATION_INFOS) {
        int32_t id = transientInfo.first;
        std::vector<int32_t> info = transientInfo.second;
        float frequencyDistance = std::abs(event.frequency - info[1]);
        for (size_t j = 0; j < TRANSIENT_GRADE_NUM; ++j) {
            float intensityDistance = std::abs(event.intensity - info[0] * (1 - j * 0.25));
            distanceInfo.push_back({id + j, intensityDistance, frequencyDistance});
        }
    }
    Normalize(distanceInfo);
    int32_t matchId = -1;
    float minWeightSum = 1;
    for (size_t i = 0; i < distanceInfo.size(); ++i) {
        float sum = INTENSITY_WEIGHT * distanceInfo[i][1] + FREQUENCY_WEIGHT * distanceInfo[i][2];
        if (sum < minWeightSum) {
            matchId = distanceInfo[i][0];
            minWeightSum = sum;
        }
    }
    if (matchId == -1) {
        MISC_HILOGE("Transient event match failed");
        return;
    }
    convertSequence_.push_back(event.delayTime);
    convertSequence_.push_back(matchId);
}

void CustomVibrationMatcher::Normalize(std::vector<std::vector<float>>& matrix)
{
    int32_t rowNum = matrix.size();
    int32_t colNum = matrix[0].size();
    std::vector<float> mins(matrix[0]);
    std::vector<float> maxs(matrix[0]);
    for (int32_t i = 0; i < rowNum; ++i) {
        for (int32_t j = 1; j < colNum; ++j) {
            mins[j] = std::min(mins[j], matrix[i][j]);
            maxs[j] = std::max(maxs[j], matrix[i][j]);
        }
    }
    for (int32_t i = 0; i < rowNum; ++i) {
        for (int32_t j = 1; j < colNum; ++j) {
            if (maxs[j] - mins[j] != 0) {
                matrix[i][j] = (matrix[i][j] - mins[j]) / (maxs[j] - mins[j]);
            } else {
                matrix[i][j] = 0;
            }
        }
    }
}

bool CustomVibrationMatcher::ParameterCheck(const std::vector<VibrateEvent>& vibrateSequence)
{
    int32_t length = vibrateSequence.size();
    if (length == 0) {
        MISC_HILOGE("the vibrateSequence array size is 0");
        return false;
    }
    for (int32_t i = 0; i < length - 1; ++i) {
        if (vibrateSequence[i + 1].delayTime < vibrateSequence[i].duration + MIN_TIME_SPACE) {
            MISC_HILOGW("The space time after the %{public}d event should be greater than 20ms", i);
        }
    }
    return true;
}
}  // namespace Sensors
}  // namespace OHOS