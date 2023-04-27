/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "custom_vibration_matcher.h"

#include <cmath>
#include <map>

#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
namespace {
std::map<int32_t, std::vector<int32_t>> TRANSIENT_VIBRATION_INFOS = {
    {0x28, {0x4d, 0x4d, 0x0b}}, {0x2c, {0x2a, 0x64, 0x07}}, {0x30, {0x44, 0x52, 0x16}},
    {0x3c, {0x45, 0x34, 0x0a}}, {0x40, {0x2e, 0x43, 0x0a}}, {0x48, {0x51, 0x52, 0x0a}},
    {0x4c, {0x3a, 0x0c, 0x0f}}, {0x50, {0x64, 0x20, 0x14}}, {0x54, {0x55, 0x34, 0x1c}},
    {0x5c, {0x32, 0x0c, 0x13}}, {0x60, {0x12, 0x07, 0x0a}}
};  // {Id, {intensity, frequency, duration}}
constexpr int32_t INTENSITY_MAX = 100;
constexpr int32_t TRANSIENT_GRADE_NUM = 4;
constexpr int32_t CONTINUOUS_GRADE_NUM = 8;
constexpr float CONTINUOUS_GRADE_SCALE = 100. / 8;
constexpr float INTENSITY_WEIGHT = 0.5;
constexpr float FREQUENCY_WEIGHT = 0.5;
constexpr float WEIGHT_SUM_INIT = 100;
constexpr int32_t STOP_WAVEFORM = 0;
constexpr int32_t EFFECT_ID_BOUNDARY = 1000;
constexpr int32_t DURATION_MAX = 1600;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "CustomVibrationMatcher" };
}  // namespace

int32_t CustomVibrationMatcher::TransformEffect(const std::set<VibrateEvent> &vibrateSet,
    std::vector<CompositeEffect> &compositeEffects)
{
    CALL_LOG_ENTER;
    int32_t preStartTime = 0;
    int32_t preDuration = 0;
    for (const auto &event : vibrateSet) {
        if ((preDuration != 0) && (event.startTime < preStartTime + preDuration)) {
            MISC_HILOGE("Vibration events overlap");
            return ERROR;
        }
        if (event.tag == EVENT_TAG_CONTINUOUS) {
            ProcessContinuousEvent(event, preStartTime, preDuration, compositeEffects);
        } else if (event.tag == EVENT_TAG_TRANSIENT) {
            ProcessTransientEvent(event, preStartTime, preDuration, compositeEffects);
        } else {
            MISC_HILOGE("Unknown event tag, tag:%{public}d", event.tag);
            return ERROR;
        }
    }
    PrimitiveEffect primitiveEffect;
    primitiveEffect.delay = preDuration;
    primitiveEffect.effectId = STOP_WAVEFORM;
    CompositeEffect compositeEffect;
    compositeEffect.primitiveEffect = primitiveEffect;
    compositeEffects.push_back(compositeEffect);
    return SUCCESS;
}

void CustomVibrationMatcher::ProcessContinuousEvent(const VibrateEvent &event, int32_t &preStartTime,
    int32_t &preDuration, std::vector<CompositeEffect> &compositeEffects)
{
    int32_t grade = -1;
    if (event.intensity == INTENSITY_MAX) {
        grade = CONTINUOUS_GRADE_NUM - 1;
    } else {
        grade = round(event.intensity / CONTINUOUS_GRADE_SCALE + 0.5) - 1;
    }
    if ((!compositeEffects.empty()) && (event.startTime == preStartTime + preDuration)) {
        PrimitiveEffect& prePrimitiveEffect = compositeEffects.back().primitiveEffect;
        int32_t preEffectId = prePrimitiveEffect.effectId;
        int32_t preGrade = preEffectId % 100;
        int32_t mergeDuration = preDuration + event.duration;
        if (preEffectId > EFFECT_ID_BOUNDARY && preGrade == grade && mergeDuration < DURATION_MAX) {
            prePrimitiveEffect.effectId = mergeDuration * 100 + grade;
            preDuration = mergeDuration;
            return;
        }
    }
    PrimitiveEffect primitiveEffect;
    primitiveEffect.delay = event.startTime - preStartTime;
    primitiveEffect.effectId = event.duration * 100 + grade;
    CompositeEffect compositeEffect;
    compositeEffect.primitiveEffect = primitiveEffect;
    compositeEffects.push_back(compositeEffect);
    preStartTime = event.startTime;
    preDuration = event.duration;
}

void CustomVibrationMatcher::ProcessTransientEvent(const VibrateEvent &event, int32_t &preStartTime,
    int32_t &preDuration, std::vector<CompositeEffect> &compositeEffects)
{
    int32_t matchId = 0;
    float minWeightSum = WEIGHT_SUM_INIT;
    for (const auto &transientInfo : TRANSIENT_VIBRATION_INFOS) {
        int32_t id = transientInfo.first;
        const std::vector<int32_t> &info = transientInfo.second;
        float frequencyDistance = std::abs(event.frequency - info[1]);
        for (int32_t j = 0; j < TRANSIENT_GRADE_NUM; ++j) {
            float intensityDistance = std::abs(event.intensity - info[0] * (1 - j * 0.25));
            float weightSum = INTENSITY_WEIGHT * intensityDistance + FREQUENCY_WEIGHT * frequencyDistance;
            if (weightSum < minWeightSum) {
                minWeightSum = weightSum;
                matchId = id + j;
            }
        }
    }
    PrimitiveEffect primitiveEffect;
    primitiveEffect.delay = event.startTime - preStartTime;
    primitiveEffect.effectId = matchId;
    CompositeEffect compositeEffect;
    compositeEffect.primitiveEffect = primitiveEffect;
    compositeEffects.push_back(compositeEffect);
    preStartTime = event.startTime;
    preDuration = event.duration;
}
}  // namespace Sensors
}  // namespace OHOS