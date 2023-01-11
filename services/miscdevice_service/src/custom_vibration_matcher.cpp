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
constexpr int32_t INTENSITY_MAX = 100;
constexpr int32_t TRANSIENT_GRADE_NUM = 4;
constexpr int32_t CONTINUOUS_GRADE_NUM = 8;
constexpr float CONTINUOUS_GRADE_SCALE = 100. / 8;
constexpr float INTENSITY_WEIGHT = 0.5;
constexpr float FREQUENCY_WEIGHT = 0.5;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "CustomVibrationMatcher" };
}  // namespace

CustomVibrationMatcher::CustomVibrationMatcher(const std::vector<VibrateEvent> &vibrateSequence)
{
    MISC_HILOGD("enter CustomVibrationMatcher");
    for (const auto &event : vibrateSequence) {
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

void CustomVibrationMatcher::ProcessContinuousEvent(const VibrateEvent &event)
{
    int32_t grade = -1;
    if (event.intensity == INTENSITY_MAX) {
        grade = CONTINUOUS_GRADE_NUM - 1;
    } else {
        grade = round(event.intensity / CONTINUOUS_GRADE_SCALE + 0.5) - 1;
    }
    if (grade < 0 || grade >= CONTINUOUS_GRADE_NUM) {
        MISC_HILOGE("Continuous event match failed, startTime : %{public}d", event.startTime);
        return;
    }
    int32_t matchId = event.duration * 100 + grade;
    int32_t delayTime = event.startTime - curPos_;
    curPos_ = event.startTime;
    convertSequence_.push_back(delayTime);
    convertSequence_.push_back(matchId);
}

void CustomVibrationMatcher::ProcessTransientEvent(const VibrateEvent &event)
{
    int32_t matchId = -1;
    float minWeightSum = 100;
    for (auto &transientInfo : TRANSIENT_VIBRATION_INFOS) {
        int32_t id = transientInfo.first;
        std::vector<int32_t> &info = transientInfo.second;
        float frequencyDistance = std::abs(event.frequency - info[1]);
        for (size_t j = 0; j < TRANSIENT_GRADE_NUM; ++j) {
            float intensityDistance = std::abs(event.intensity - info[0] * (1 - j * 0.25));
            float weightSum = INTENSITY_WEIGHT * intensityDistance + FREQUENCY_WEIGHT * frequencyDistance;
            if (weightSum < minWeightSum) {
                minWeightSum = weightSum;
                matchId = id + j;
            }
        }
    }
    if (matchId == -1) {
        MISC_HILOGE("Transient event match failed, startTime : %{public}d", event.startTime);
        return;
    }
    int32_t delayTime = event.startTime - curPos_;
    curPos_ = event.startTime;
    convertSequence_.push_back(delayTime);
    convertSequence_.push_back(matchId);
}
}  // namespace Sensors
}  // namespace OHOS