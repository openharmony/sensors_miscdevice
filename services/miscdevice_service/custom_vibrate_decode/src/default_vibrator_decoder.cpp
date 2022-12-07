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
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "DefaultVibratorDecoder" };
} // namespace

int32_t DefaultVibratorDecoder::DecodeEffect(const int32_t fd, std::vector<VibrateEvent>& vibrateSequence)
{
    cJSON* cJson = GetFdCJson(fd);
    int32_t ret = ParseSequence(cJson, vibrateSequence);
    CHKCR((ret == SUCCESS), ERROR, "parse sequence fail");
    cJSON_Delete(cJson);
    return SUCCESS;
}

int32_t DefaultVibratorDecoder::ParseSequence(cJSON* cJson, std::vector<VibrateEvent>& vibrateSequence)
{
    CHKPR(cJson, ERROR);
    cJSON* sequence = cJSON_GetObjectItem(cJson, "Sequence");
    CHKPR(sequence, ERROR);
    if (!cJSON_IsArray(sequence)) {
        MISC_HILOGE("The value of sequence is not array");
        return ERROR;
    }
    int32_t size = cJSON_GetArraySize(sequence);
    int32_t preStartTime = 0;
    for (int32_t i = 0; i < size; i++) {
        cJSON* event = cJSON_GetArrayItem(sequence, i)->child;
        int32_t ret = ParseEvent(event, vibrateSequence, preStartTime);
        CHKCR((ret == SUCCESS), ERROR, "parse event fail");
    }
    return SUCCESS;
}

int32_t DefaultVibratorDecoder::ParseEvent(cJSON* event, std::vector<VibrateEvent>& vibrateSequence,
                                           int32_t& preStartTime)
{
    CHKPR(event, ERROR);
    VibrateEvent vibrateEvent;
    cJSON* type = cJSON_GetObjectItem(event, "Type");
    CHKPR(type, ERROR);
    std::string curType = type->valuestring;
    if (curType == "continuous") {
        vibrateEvent.tag = CONTINUOUS;
        cJSON* duration = cJSON_GetObjectItem(event, "Duration");
        CHKPR(duration, ERROR);
        vibrateEvent.duration = duration->valueint;
    } else if (curType == "transient") {
        vibrateEvent.tag = TRANSIENT;
        vibrateEvent.duration = 30;
    } else {
        MISC_HILOGE("current event type is not continuous or transient");
        return ERROR;
    }
    cJSON* startTime = cJSON_GetObjectItem(event, "StartTime");
    CHKPR(startTime, ERROR);
    int32_t curStartTime = startTime->valueint;
    vibrateEvent.delayTime = curStartTime - preStartTime;
    preStartTime = curStartTime;
    cJSON* intensity = cJSON_GetObjectItem(event, "Intensity");
    CHKPR(intensity, ERROR);
    vibrateEvent.intensity = intensity->valueint;
    cJSON* frequency = cJSON_GetObjectItem(event, "Frequency");
    CHKPR(frequency, ERROR);
    vibrateEvent.frequency = frequency->valueint;
    vibrateSequence.push_back(vibrateEvent);
    return SUCCESS;
}

}  // namespace Sensors
}  // namespace OHOS