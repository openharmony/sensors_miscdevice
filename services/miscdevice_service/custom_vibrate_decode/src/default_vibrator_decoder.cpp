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
constexpr double SUPPORT_JSON_VERSION = 1.0;
constexpr int32_t SUPPORT_CHANNEL_NUMBER = 1;
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "DefaultVibratorDecoder" };
} // namespace

std::set<VibrateEvent> DefaultVibratorDecoder::DecodeEffect(const RawFileDescriptor &rawFd)
{
    JsonParser parser(rawFd);
    int32_t ret = CheckMetadata(parser);
    if (ret != SUCCESS) {
        MISC_HILOGE("check metadata fail");
        return {};
    }
    std::set<VibrateEvent> vibrateSet;
    ret = ParseChannel(parser, vibrateSet);
    if (ret != SUCCESS) {
        MISC_HILOGE("parse channel fail");
        return {};
    }
    return vibrateSet;
}

int32_t DefaultVibratorDecoder::CheckMetadata(const JsonParser &parser)
{
    cJSON* metadata = parser.GetObjectItem("MetaData");
    CHKPR(metadata, ERROR);
    cJSON* version = parser.GetObjectItem(metadata, "Version");
    CHKPR(version, ERROR);
    version_ = version->valuedouble;
    if (version_ != SUPPORT_JSON_VERSION) {
        MISC_HILOGE("json file version is not supported, version:%{public}f", version_);
        return ERROR;
    }
    cJSON* channelNumber = parser.GetObjectItem(metadata, "ChannelNumber");
    CHKPR(channelNumber, ERROR);
    channelNumber_ = channelNumber->valueint;
    if (channelNumber_ != SUPPORT_CHANNEL_NUMBER) {
        MISC_HILOGE("json file channelNumber is not supported, channelNumber:%{public}d", channelNumber_);
        return ERROR;
    }
    return SUCCESS;
}

int32_t DefaultVibratorDecoder::ParseChannel(const JsonParser &parser, std::set<VibrateEvent> &vibrateSet)
{
    cJSON *channels = parser.GetObjectItem("Channels");
    CHKPR(channels, ERROR);
    if (!parser.IsArray(channels)) {
        MISC_HILOGE("The value of channels is not array");
        return ERROR;
    }
    int32_t size = parser.GetArraySize(channels);
    if (size != channelNumber_) {
        MISC_HILOGE("The size of channels conflicts with channelNumber, size:%{public}d", size);
        return ERROR;
    }
    for (int32_t i = 0; i < size; i++) {
        cJSON* channel = parser.GetArrayItem(channels, i);
        CHKPR(channel, ERROR);
        cJSON* channelParameters = parser.GetObjectItem(channel, "Parameters");
        CHKPR(channelParameters, ERROR);
        int32_t ret = ParseChannelParameters(parser, channelParameters);
        CHKCR((ret == SUCCESS), ERROR, "parse channel parameters fail");
        cJSON* pattern = parser.GetObjectItem(channel, "Pattern");
        CHKPR(pattern, ERROR);
        ret = ParsePattern(parser, pattern, vibrateSet);
        CHKCR((ret == SUCCESS), ERROR, "parse pattern fail");
    }
    return SUCCESS;
}

int32_t DefaultVibratorDecoder::ParseChannelParameters(const JsonParser &parser, cJSON *channelParameters)
{
    cJSON* index = parser.GetObjectItem(channelParameters, "Index");
    CHKPR(index, ERROR);
    int32_t indexVal = index->valueint;
    CHKCR((indexVal == SUPPORT_CHANNEL_NUMBER), ERROR, "invalid channel index");
    return SUCCESS;
}

int32_t DefaultVibratorDecoder::ParsePattern(const JsonParser &parser, cJSON *pattern,
    std::set<VibrateEvent> &vibrateSet)
{
    if (!parser.IsArray(pattern)) {
        MISC_HILOGE("The value of pattern is not array");
        return ERROR;
    }
    int32_t size = parser.GetArraySize(pattern);
    if (size > EVENT_NUM_MAX) {
        MISC_HILOGE("The size of pattern is out of bounds, size:%{public}d", size);
        return ERROR;
    }
    for (int32_t i = 0; i < size; i++) {
        cJSON* item = parser.GetArrayItem(pattern, i);
        CHKPR(item, ERROR);
        cJSON* event = parser.GetObjectItem(item, "Event");
        CHKPR(event, ERROR);
        int32_t ret = ParseEvent(parser, event, vibrateSet);
        CHKCR((ret == SUCCESS), ERROR, "parse event fail");
    }
    return SUCCESS;
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
        MISC_HILOGE("Unknown event type, curType:%{public}s", curType.c_str());
        return ERROR;
    }
    cJSON* startTime = parser.GetObjectItem(event, "StartTime");
    CHKPR(startTime, ERROR);
    vibrateEvent.startTime = startTime->valueint;
    cJSON* eventParameters = parser.GetObjectItem(event, "Parameters");
    CHKPR(eventParameters, ERROR);
    cJSON* intensity = parser.GetObjectItem(eventParameters, "Intensity");
    CHKPR(intensity, ERROR);
    vibrateEvent.intensity = intensity->valueint;
    cJSON* frequency = parser.GetObjectItem(eventParameters, "Frequency");
    CHKPR(frequency, ERROR);
    vibrateEvent.frequency = frequency->valueint;
    if (!CheckParameters(vibrateEvent)) {
        MISC_HILOGE("Parameter check of vibration event failed, startTime:%{public}d", vibrateEvent.startTime);
        return ERROR;
    }
    if (vibrateEvent.intensity == INTENSITY_MIN) {
        MISC_HILOGI("The event intensity is 0, startTime:%{public}d", vibrateEvent.startTime);
        return SUCCESS;
    }
    auto ret = vibrateSet.insert(vibrateEvent);
    if (!ret.second) {
        MISC_HILOGE("Vibration event is duplicated, startTime:%{public}d", vibrateEvent.startTime);
        return ERROR;
    }
    return SUCCESS;
}

bool DefaultVibratorDecoder::CheckParameters(const VibrateEvent &vibrateEvent)
{
    if (vibrateEvent.startTime < STARTTMIE_MIN || vibrateEvent.startTime > STARTTMIE_MAX) {
        MISC_HILOGE("the event startTime is out of range, startTime:%{public}d", vibrateEvent.startTime);
        return false;
    }
    if (vibrateEvent.duration <= CONTINUOUS_VIBRATION_DURATION_MIN ||
        vibrateEvent.duration >= CONTINUOUS_VIBRATION_DURATION_MAX) {
        MISC_HILOGE("the event duration is out of range, duration:%{public}d", vibrateEvent.duration);
        return false;
    }
    if (vibrateEvent.intensity < INTENSITY_MIN || vibrateEvent.intensity > INTENSITY_MAX) {
        MISC_HILOGE("the event intensity is out of range, intensity:%{public}d", vibrateEvent.intensity);
        return false;
    }
    if (vibrateEvent.frequency < FREQUENCY_MIN || vibrateEvent.frequency > FREQUENCY_MAX) {
        MISC_HILOGE("the event frequency is out of range, frequency:%{public}d", vibrateEvent.frequency);
        return false;
    }
    return true;
}
}  // namespace Sensors
}  // namespace OHOS