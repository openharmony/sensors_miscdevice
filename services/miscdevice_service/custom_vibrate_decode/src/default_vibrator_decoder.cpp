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

#include "default_vibrator_decoder.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr int32_t STARTTMIE_MIN = 0;
constexpr int32_t STARTTMIE_MAX = 1800000;
constexpr int32_t CONTINUOUS_VIBRATION_DURATION_MIN = 10;
constexpr int32_t CONTINUOUS_VIBRATION_DURATION_MAX = 1600;
constexpr int32_t TRANSIENT_VIBRATION_DURATION = 30;
constexpr int32_t INTENSITY_MIN = 0;
constexpr int32_t INTENSITY_MAX = 100;
constexpr int32_t FREQUENCY_MIN = 0;
constexpr int32_t FREQUENCY_MAX = 100;
constexpr int32_t EVENT_NUM_MAX = 128;
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "DefaultVibratorDecoder" };
} // namespace

int32_t DefaultVibratorDecoder::DecodeEffect(const JsonParser &parser, std::set<VibrateEvent> &vibrateSet)
{
    int32_t ret = ParseSequence(parser, vibrateSet);
    CHKCR((ret == SUCCESS), ERROR, "parse sequence fail");
    return SUCCESS;
}

int32_t DefaultVibratorDecoder::ParseSequence(const JsonParser &parser, std::set<VibrateEvent> &vibrateSet)
{
    cJSON *sequence = parser.GetObjectItem("Sequence");
    CHKPR(sequence, ERROR);
    if (!parser.IsArray(sequence)) {
        MISC_HILOGE("The value of sequence is not array");
        return ERROR;
    }
    int32_t size = parser.GetArraySize(sequence);
    if (size > EVENT_NUM_MAX) {
        MISC_HILOGE("The size of sequence exceeds 128, size: %{public}d", size);
        return ERROR;
    }
    for (int32_t i = 0; i < size; i++) {
        cJSON* item = parser.GetArrayItem(sequence, i);
        CHKPR(item, ERROR);
        cJSON* event = item->child;
        CHKPR(event, ERROR);
        int32_t ret = ParseEvent(parser, event, vibrateSet);
        CHKCR((ret == SUCCESS), ERROR, "parse event fail");
    }
    return SUCCESS;
}

bool CheckParameters(const VibrateEvent &vibrateEvent, const std::set<VibrateEvent> &vibrateSet)
{
    if (vibrateEvent.startTime < STARTTMIE_MIN || vibrateEvent.startTime > STARTTMIE_MAX) {
        MISC_HILOGE("the event startTime is out of range, startTime: %{public}d", vibrateEvent.startTime);
        return false;
    }
    if (vibrateEvent.duration <= CONTINUOUS_VIBRATION_DURATION_MIN ||
        vibrateEvent.duration >= CONTINUOUS_VIBRATION_DURATION_MAX) {
        MISC_HILOGE("the event duration is out of range, duration: %{public}d", vibrateEvent.duration);
        return false;
    }
    if (vibrateEvent.intensity <= INTENSITY_MIN || vibrateEvent.intensity > INTENSITY_MAX) {
        MISC_HILOGE("the event intensity is out of range, intensity: %{public}d", vibrateEvent.intensity);
        return false;
    }
    if (vibrateEvent.frequency <= FREQUENCY_MIN || vibrateEvent.frequency > FREQUENCY_MAX) {
        MISC_HILOGE("the event frequency is out of range, frequency: %{public}d", vibrateEvent.frequency);
        return false;
    }
    return true;
}

int32_t DefaultVibratorDecoder::ParseEvent(const JsonParser &parser, cJSON *event,
                                           std::set<VibrateEvent> &vibrateSet)
{
    VibrateEvent vibrateEvent;
    cJSON* type = parser.GetObjectItem(event, "Type");
    CHKPR(type, ERROR);
    std::string curType = type->valuestring;
    if (curType == "continuous") {
        vibrateEvent.tag = EVENT_TAG_CONTINUOUS;
        cJSON* duration = parser.GetObjectItem(event, "Duration");
        CHKPR(duration, ERROR);
        vibrateEvent.duration = duration->valueint;
    } else if (curType == "transient") {
        vibrateEvent.tag = EVENT_TAG_TRANSIENT;
        vibrateEvent.duration = TRANSIENT_VIBRATION_DURATION;
    } else {
        MISC_HILOGE("Unknown event type, curType: %{public}s", curType.c_str());
        return ERROR;
    }
    cJSON* startTime = parser.GetObjectItem(event, "StartTime");
    CHKPR(startTime, ERROR);
    vibrateEvent.startTime = startTime->valueint;
    cJSON* intensity = parser.GetObjectItem(event, "Intensity");
    CHKPR(intensity, ERROR);
    vibrateEvent.intensity = intensity->valueint;
    cJSON* frequency = parser.GetObjectItem(event, "Frequency");
    CHKPR(frequency, ERROR);
    vibrateEvent.frequency = frequency->valueint;
    if (!CheckParameters(vibrateEvent, vibrateSet)) {
        MISC_HILOGE("Parameter check of vibration event failed, startTime: %{public}d.", vibrateEvent.startTime);
        return ERROR;
    }
    auto ret = vibrateSet.insert(vibrateEvent);
    if (!ret.second) {
        MISC_HILOGE("Vibration event repeated insertion, startTime: %{public}d.", vibrateEvent.startTime);
        return ERROR;
    }
    return SUCCESS;
}
}  // namespace Sensors
}  // namespace OHOS