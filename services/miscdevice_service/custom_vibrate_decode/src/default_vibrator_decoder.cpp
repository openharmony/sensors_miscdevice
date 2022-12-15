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
constexpr int32_t INTENSITY_MIN = 0;
constexpr int32_t INTENSITY_MAX = 100;
constexpr int32_t FREQUENCY_MIN = -100;
constexpr int32_t FREQUENCY_MAX = 100;
constexpr int32_t MIN_VIBRATE_DURATION = 30;
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "DefaultVibratorDecoder" };
} // namespace

int32_t DefaultVibratorDecoder::DecodeEffect(int32_t fd, std::vector<VibrateEvent> &vibrateSequence)
{
    JsonParser parser(fd);
    int32_t ret = ParseSequence(parser, vibrateSequence);
    CHKCR((ret == SUCCESS), ERROR, "parse sequence fail");
    return SUCCESS;
}

int32_t DefaultVibratorDecoder::ParseSequence(const JsonParser &parser, std::vector<VibrateEvent> &vibrateSequence)
{
    cJSON *sequence = parser.GetObjectItem("Sequence");
    CHKPR(sequence, ERROR);
    if (!cJSON_IsArray(sequence)) {
        MISC_HILOGE("The value of sequence is not array");
        return ERROR;
    }
    int32_t size = cJSON_GetArraySize(sequence);
    int32_t preStartTime = 0;
    for (int32_t i = 0; i < size; i++) {
        cJSON* event = cJSON_GetArrayItem(sequence, i)->child;
        CHKPR(event, ERROR);
        int32_t ret = ParseEvent(parser, event, vibrateSequence, preStartTime);
        CHKCR((ret == SUCCESS), ERROR, "parse event fail");
    }
    return SUCCESS;
}

int32_t DefaultVibratorDecoder::ParseEvent(const JsonParser &parser, cJSON *event,
                                           std::vector<VibrateEvent> &vibrateSequence, int32_t &preStartTime)
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
        vibrateEvent.duration = MIN_VIBRATE_DURATION;
    } else {
        MISC_HILOGE("current event type is not continuous or transient");
        return ERROR;
    }
    cJSON* startTime = parser.GetObjectItem(event, "StartTime");
    CHKPR(startTime, ERROR);
    int32_t curStartTime = startTime->valueint;
    vibrateEvent.delayTime = curStartTime - preStartTime;
    preStartTime = curStartTime;
    cJSON* intensity = parser.GetObjectItem(event, "Intensity");
    CHKPR(intensity, ERROR);
    vibrateEvent.intensity = intensity->valueint;
    if (vibrateEvent.intensity <= INTENSITY_MIN || vibrateEvent.intensity > INTENSITY_MAX) {
        MISC_HILOGE("the event intensity is not in (0 ~ 100], intensity: %{public}d", vibrateEvent.intensity);
        return ERROR;
    }
    cJSON* frequency = parser.GetObjectItem(event, "Frequency");
    CHKPR(frequency, ERROR);
    vibrateEvent.frequency = frequency->valueint;
    if (vibrateEvent.frequency < FREQUENCY_MIN || vibrateEvent.frequency > FREQUENCY_MAX) {
        MISC_HILOGE("the event frequency is not in [-100 ~ 100], frequency: %{public}d", vibrateEvent.frequency);
        return ERROR;
    }
    vibrateSequence.push_back(vibrateEvent);
    return SUCCESS;
}

}  // namespace Sensors
}  // namespace OHOS