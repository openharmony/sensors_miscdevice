/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <map>
#include <set>

#include "ohos.vibrator.proj.hpp"
#include "ohos.vibrator.impl.hpp"
#include "stdexcept"
#include "taihe/runtime.hpp"

#include "file_utils.h"
#include "miscdevice_log.h"
#include "sensors_errors.h"
#include "vibrator_agent.h"

#undef LOG_TAG
#define LOG_TAG "VibratorImpl"

namespace {
using namespace OHOS::Sensors;

constexpr int32_t INTENSITY_ADJUST_MAX = 100;
constexpr int32_t EVENT_START_TIME_MAX = 1800000;
constexpr int32_t EVENT_NUM_MAX = 128;
constexpr int32_t INTENSITY_MIN = 0;
constexpr int32_t INTENSITY_MAX = 100;
constexpr int32_t FREQUENCY_MIN = 0;
constexpr int32_t FREQUENCY_MAX = 100;
constexpr int32_t CURVE_POINT_INTENSITY_MAX = 100;
constexpr int32_t CURVE_POINT_NUM_MIN = 4;
constexpr int32_t CURVE_POINT_NUM_MAX = 16;
constexpr int32_t CURVE_FREQUENCY_MIN = -100;
constexpr int32_t CURVE_FREQUENCY_MAX = 100;
constexpr int32_t CONTINUOUS_DURATION_MAX = 5000;
constexpr int32_t EVENT_INDEX_MAX = 2;

static std::map<std::string, int32_t> g_usageType = {
    {"unknown", USAGE_UNKNOWN},
    {"alarm", USAGE_ALARM},
    {"ring", USAGE_RING},
    {"notification", USAGE_NOTIFICATION},
    {"communication", USAGE_COMMUNICATION},
    {"touch", USAGE_TOUCH},
    {"media", USAGE_MEDIA},
    {"physicalFeedback", USAGE_PHYSICAL_FEEDBACK},
    {"simulateReality", USAGE_SIMULATE_REALITY},
};

static std::set<std::string> g_allowedTypes = {"time", "preset", "file", "pattern"};

struct VibrateInfo {
    std::string type;
    std::string usage;
    bool systemUsage {false};
    int32_t duration = 0;
    std::string effectId;
    int32_t count = 0;
    int32_t fd = -1;
    int64_t offset = 0;
    int64_t length = -1;
    int32_t intensity = 0;
    VibratorPattern vibratorPattern;
};

void stopVibrationByModeSync(::ohos::vibrator::VibratorStopMode stopMode)
{
    CALL_LOG_ENTER;
    StopVibrator(stopMode.get_value());
}

static bool ParseVibratorCurvePoint(::taihe::array<::ohos::vibrator::VibratorCurvePoint> pointsArray, uint32_t index,
    VibratorCurvePoint &point)
{
    CALL_LOG_ENTER;
    point.time = pointsArray[index].time;
    point.intensity = pointsArray[index].intensity.has_value() ? pointsArray[index].intensity.value() : 0;
    point.frequency = pointsArray[index].frequency.has_value() ? pointsArray[index].frequency.value() : 0;
    return true;
}

static bool ParseVibratorCurvePointArray(::taihe::array<::ohos::vibrator::VibratorCurvePoint> pointsArray,
    uint32_t pointsLength, VibratorEvent &event)
{
    CALL_LOG_ENTER;
    if (pointsLength <= 0 || pointsLength > CURVE_POINT_NUM_MAX) {
        MISC_HILOGE("pointsLength should not be less than or equal to 0 or greater than CURVE_POINT_NUM_MAX");
        return false;
    }
    VibratorCurvePoint *points =
        static_cast<VibratorCurvePoint *>(malloc(sizeof(VibratorCurvePoint) * pointsLength));
    if (points == nullptr) {
        MISC_HILOGE("points is nullptr");
        return false;
    }
    for (uint32_t i = 0; i < pointsLength; ++i) {
        if (!ParseVibratorCurvePoint(pointsArray, i, points[i])) {
            MISC_HILOGE("ParseVibratorCurvePoint failed");
            free(points);
            points = nullptr;
            return false;
        }
    }
    event.points = points;
    return true;
}

static bool ParseVibrateEvent(::taihe::array<::ohos::vibrator::VibratorEvent> eventArray, int32_t index,
    VibratorEvent &event)
{
    CALL_LOG_ENTER;
    ::ohos::vibrator::VibratorEvent vibratorEvent = eventArray[index];
    event.type = static_cast<VibratorEventType>(static_cast<int32_t>(vibratorEvent.eventType));
    event.time = vibratorEvent.time;
    event.duration = vibratorEvent.duration.has_value() ? vibratorEvent.duration.value() : 0;
    event.intensity = vibratorEvent.intensity.has_value() ? vibratorEvent.intensity.value() : 0;
    event.frequency = vibratorEvent.frequency.has_value() ? vibratorEvent.frequency.value() : 0;
    event.index = vibratorEvent.index.has_value() ? vibratorEvent.index.value() : 0;
    if (!vibratorEvent.points.has_value()) {
        MISC_HILOGI("has no points");
        return true;
    }
    ::taihe::array<::ohos::vibrator::VibratorCurvePoint> points = vibratorEvent.points.value();
    std::size_t size = points.size();
    event.pointNum = static_cast<int32_t>(size);
    if (size <= 0) {
        MISC_HILOGI("has zero points");
        return true;
    }
    if (!ParseVibratorCurvePointArray(points, event.pointNum, event)) {
        MISC_HILOGE("ParseVibratorCurvePointArray failed");
        return false;
    }
    return true;
}

static bool ParseVibratorPattern(::ohos::vibrator::VibratorPattern pattern, VibrateInfo &vibrateInfo)
{
    CALL_LOG_ENTER;
    vibrateInfo.vibratorPattern.time = pattern.time;
    vibrateInfo.vibratorPattern.eventNum = static_cast<int32_t>(pattern.events.size());
    if (vibrateInfo.vibratorPattern.eventNum <= 0 || vibrateInfo.vibratorPattern.eventNum > EVENT_NUM_MAX) {
        MISC_HILOGE("length should not be less than or equal to 0 or greater than EVENT_NUM_MAX");
        return false;
    }
    VibratorEvent *events =
        static_cast<VibratorEvent *>(malloc(sizeof(VibratorEvent) * vibrateInfo.vibratorPattern.eventNum));
    if (events == nullptr) {
        MISC_HILOGE("Events is nullptr");
        return false;
    }
    for (int32_t j = 0; j < vibrateInfo.vibratorPattern.eventNum; ++j) {
        new (&events[j]) VibratorEvent();
        if (!ParseVibrateEvent(pattern.events, j, events[j])) {
            MISC_HILOGE("ParseVibrateEvent failed");
            free(events);
            events = nullptr;
            return false;
        }
    }
    vibrateInfo.vibratorPattern.events = events;
    return true;
}

static void PrintVibratorPattern(::ohos::vibrator::VibratorPattern &vibratorPattern)
{
    CALL_LOG_ENTER;
    if (vibratorPattern.events.empty()) {
        MISC_HILOGE("Events is empty");
        return;
    }
    MISC_HILOGD("PrintVibratorPattern, time:%{public}d, eventNum:%{public}u",
        vibratorPattern.time, vibratorPattern.events.size());
    for (int32_t i = 0; i < static_cast<int32_t>(vibratorPattern.events.size()); ++i) {
        MISC_HILOGD("PrintVibratorPattern, type:%{public}d, time:%{public}d, duration:%{public}d, \
            intensity:%{public}d, frequency:%{public}d, index:%{public}d, pointNum:%{public}u",
            static_cast<int32_t>(vibratorPattern.events[i].eventType), vibratorPattern.events[i].time,
            vibratorPattern.events[i].duration.value(), vibratorPattern.events[i].intensity.value(),
            vibratorPattern.events[i].frequency.value(), vibratorPattern.events[i].index.value(),
            vibratorPattern.events[i].points.value().size());
        if ((!vibratorPattern.events[i].points.has_value()) || vibratorPattern.events[i].points.value().size() <= 0) {
            MISC_HILOGE("points is empty");
            return;
        }
        ::taihe::array<::ohos::vibrator::VibratorCurvePoint> point = vibratorPattern.events[i].points.value();
        for (int32_t j = 0; j < static_cast<int32_t>(vibratorPattern.events[i].points.value().size()); ++j) {
            MISC_HILOGD("PrintVibratorPattern, time:%{public}d, intensity:%{public}d, frequency:%{public}d",
                point[j].time, point[j].intensity.value(), point[j].frequency.value());
        }
    }
}

static bool CheckVibratorCurvePoint(const ::ohos::vibrator::VibratorEvent &event)
{
    CALL_LOG_ENTER;
    if ((!event.points.has_value()) || (event.points.value().size() < CURVE_POINT_NUM_MIN) ||
        (event.points.value().size() > CURVE_POINT_NUM_MAX)) {
        MISC_HILOGE("The points size is out of range, pointNum:%{public}u", event.points.value().size());
        return false;
    }
    for (int32_t j = 0; j < static_cast<int32_t>(event.points.value().size()); ++j) {
        if ((event.points.value()[j].time < 0) || (event.points.value()[j].time > event.duration.value())) {
            MISC_HILOGE("time in points is out of range, time:%{public}d", event.points.value()[j].time);
            return false;
        }
        if ((event.points.value()[j].intensity.value() < 0) ||
            (event.points.value()[j].intensity.value() > CURVE_POINT_INTENSITY_MAX)) {
            MISC_HILOGE("intensity in points is out of range, intensity:%{public}d",
                event.points.value()[j].intensity.value());
            return false;
        }
        if ((event.points.value()[j].frequency.value() < CURVE_FREQUENCY_MIN) ||
            (event.points.value()[j].frequency.value() > CURVE_FREQUENCY_MAX)) {
            MISC_HILOGE("frequency in points is out of range, frequency:%{public}d",
                event.points.value()[j].frequency.value());
            return false;
        }
    }
    return true;
}

static bool CheckVibratorEvent(const ::ohos::vibrator::VibratorEvent &event)
{
    CALL_LOG_ENTER;
    if ((event.time < 0) || (event.time > EVENT_START_TIME_MAX)) {
        MISC_HILOGE("The event time is out of range, time:%{public}d", event.time);
        return false;
    }
    if ((!event.frequency.has_value()) || (event.frequency.value() < FREQUENCY_MIN) ||
        (event.frequency.value() > FREQUENCY_MAX)) {
        MISC_HILOGE("The event frequency is out of range, frequency:%{public}d", event.frequency.value());
        return false;
    }
    if ((!event.intensity.has_value()) || (event.intensity.value() < INTENSITY_MIN) ||
        (event.intensity.value() > INTENSITY_MAX)) {
        MISC_HILOGE("The event intensity is out of range, intensity:%{public}d", event.intensity.value());
        return false;
    }
    if ((!event.duration.has_value()) || (event.duration.value() <= 0) ||
        (event.duration.value() > CONTINUOUS_DURATION_MAX)) {
        MISC_HILOGE("The event duration is out of range, duration:%{public}d", event.duration.value());
        return false;
    }
    if ((!event.index.has_value()) || (event.index.value() < 0) || (event.index.value() > EVENT_INDEX_MAX)) {
        MISC_HILOGE("The event index is out of range, index:%{public}d", event.index.value());
        return false;
    }
    if ((event.eventType == VibratorEventType::EVENT_TYPE_CONTINUOUS) && (event.points.value().size() > 0)) {
        if (!CheckVibratorCurvePoint(event)) {
            MISC_HILOGE("CheckVibratorCurvePoint failed");
            return false;
        }
    }
    return true;
}

static bool CheckVibratorPatternParameter(::ohos::vibrator::VibratorPattern &vibratorPattern)
{
    CALL_LOG_ENTER;
    if (vibratorPattern.events.empty()) {
        MISC_HILOGE("Events is empty");
        return false;
    }
    if ((vibratorPattern.events.size() <= 0) || (vibratorPattern.events.size() > EVENT_NUM_MAX)) {
        MISC_HILOGE("The event num  is out of range, eventNum:%{public}u", vibratorPattern.events.size());
        return false;
    }
    for (int32_t i = 0; i < static_cast<int32_t>(vibratorPattern.events.size()); ++i) {
        if (!CheckVibratorEvent(vibratorPattern.events[i])) {
            MISC_HILOGE("CheckVibratorEvent failed");
            return false;
        }
    }
    return true;
}

bool ParseParameter(::ohos::vibrator::VibrateEffect const& effect, ::ohos::vibrator::VibrateAttribute const& attribute,
    VibrateInfo &vibrateInfo)
{
    CALL_LOG_ENTER;
    if (effect.holds_VibrateTime_type()) {
        vibrateInfo.type = "time";
        ::ohos::vibrator::VibrateTime vibrateTime = effect.get_VibrateTime_type_ref();
        vibrateInfo.duration = vibrateTime.duration;
    } else if (effect.holds_VibratePreset_type()) {
        vibrateInfo.type = "preset";
        ::ohos::vibrator::VibratePreset vibratePreset = effect.get_VibratePreset_type_ref();
        vibrateInfo.effectId = vibratePreset.effectId;
        vibrateInfo.count = vibratePreset.count.has_value() ? vibratePreset.count.value() : 1;
        vibrateInfo.intensity = vibratePreset.intensity.has_value() ? vibratePreset.intensity.value() :
            INTENSITY_ADJUST_MAX;
    } else if (effect.holds_VibrateFromFile_type()) {
        vibrateInfo.type = "file";
        ::ohos::vibrator::VibrateFromFile vibrateFromFile = effect.get_VibrateFromFile_type_ref();
        vibrateInfo.fd = vibrateFromFile.hapticFd.fd;
        vibrateInfo.offset = vibrateFromFile.hapticFd.offset.has_value() ? vibrateFromFile.hapticFd.offset.value() : 0;
        int64_t fdSize = GetFileSize(vibrateInfo.fd);
        CHKCR((vibrateInfo.offset >= 0) && (vibrateInfo.offset <= fdSize), false, "The parameter of offset is invalid");
        vibrateInfo.length = vibrateFromFile.hapticFd.length.has_value() ? vibrateFromFile.hapticFd.length.value() :
            fdSize - vibrateInfo.offset;
    } else if (effect.holds_VibrateFromPattern_type()) {
        vibrateInfo.type = "pattern";
        ::ohos::vibrator::VibrateFromPattern vibrateFromPattern = effect.get_VibrateFromPattern_type_ref();
        ParseVibratorPattern(vibrateFromPattern.pattern, vibrateInfo);
        PrintVibratorPattern(vibrateFromPattern.pattern);
        CHKCF(CheckVibratorPatternParameter(vibrateFromPattern.pattern), "CheckVibratorPatternParameter fail");
    }
    vibrateInfo.usage = attribute.usage;
    vibrateInfo.systemUsage = attribute.systemUsage.has_value() ? attribute.systemUsage.value() : false;
    return true;
}

bool SetUsageInner(const std::string &usage, bool systemUsage)
{
    CALL_LOG_ENTER;
    if (auto iter = g_usageType.find(usage); iter == g_usageType.end()) {
        taihe::set_business_error(PARAMETER_ERROR, "Wrong usage type " + usage);
        return false;
    }
    return SetUsage(g_usageType[usage], systemUsage);
}

int32_t CheckParameters(const VibrateInfo &info)
{
    CALL_LOG_ENTER;
    if (!SetUsageInner(info.usage, info.systemUsage)) {
        MISC_HILOGE("SetUsage fail");
        taihe::set_business_error(PARAMETER_ERROR, "Parameters invalid");
        return PARAMETER_ERROR;
    }
    if (g_allowedTypes.find(info.type) == g_allowedTypes.end()) {
        MISC_HILOGE("Invalid vibrate type, type:%{public}s", info.type.c_str());
        taihe::set_business_error(PARAMETER_ERROR, "Parameters invalid");
        return PARAMETER_ERROR;
    }
    return OHOS::ERR_OK;
}

int32_t StartVibrate(const VibrateInfo &info)
{
    CALL_LOG_ENTER;
    if (CheckParameters(info) != OHOS::ERR_OK) {
        taihe::set_business_error(PARAMETER_ERROR, "Parameters invalid");
        MISC_HILOGE("CheckParameters fail");
        return PARAMETER_ERROR;
    }
    if (info.type == "file") {
        return PlayVibratorCustom(info.fd, info.offset, info.length);
    } else if (info.type == "pattern") {
        return PlayPattern(info.vibratorPattern);
    }
    return StartVibratorOnce(info.duration);
}

bool ClearVibratorPattern(VibratorPattern &vibratorPattern)
{
    CALL_LOG_ENTER;
    int32_t eventSize = vibratorPattern.eventNum;
    if ((eventSize <= 0) || (vibratorPattern.events == nullptr)) {
        MISC_HILOGW("events is not need to free, eventSize:%{public}d", eventSize);
        return false;
    }
    auto events = vibratorPattern.events;
    for (int32_t j = 0; j < eventSize; ++j) {
        if (events[j].points != nullptr) {
            free(events[j].points);
            events[j].points = nullptr;
        }
    }
    free(events);
    events = nullptr;
    return true;
}

void startVibrationSync(::ohos::vibrator::VibrateEffect const& effect,
    ::ohos::vibrator::VibrateAttribute const& attribute)
{
    CALL_LOG_ENTER;
    VibrateInfo vibrateInfo = {};
    if (!ParseParameter(effect, attribute, vibrateInfo)) {
        MISC_HILOGE("ParseParameter fail");
        taihe::set_business_error(PARAMETER_ERROR, "ParseParameter fail");
        return;
    }
    if (vibrateInfo.type == "preset") {
        if (!SetLoopCount(vibrateInfo.count) || CheckParameters(vibrateInfo) != OHOS::ERR_OK ||
            vibrateInfo.effectId.empty()) {
            MISC_HILOGE("SetLoopCount fail or parameter invalid");
            taihe::set_business_error(PARAMETER_ERROR, "SetLoopCount fail or parameter invalid");
            return;
        }
        int32_t ret = PlayPrimitiveEffect(vibrateInfo.effectId.c_str(), vibrateInfo.intensity);
        if (ret != SUCCESS) {
            taihe::set_business_error(PARAMETER_ERROR, "start vibrator failed");
            return;
        }
    } else {
        int32_t ret = StartVibrate(vibrateInfo);
        if (vibrateInfo.vibratorPattern.events != nullptr) {
            CHKCV(ClearVibratorPattern(vibrateInfo.vibratorPattern), "ClearVibratorPattern fail");
        }
        if (ret == PARAMETER_ERROR) {
            taihe::set_business_error(PARAMETER_ERROR, "Parameters invalid");
            return;
        }
    }
}

bool isHdHapticSupported()
{
    CALL_LOG_ENTER;
    return IsHdHapticSupported();
}

void stopVibrationSync()
{
    CALL_LOG_ENTER;
    int32_t ret = Cancel();
    if (ret != SUCCESS) {
        taihe::set_business_error(ret, "Cancel execution fail");
    }
}

bool isSupportEffectSync(::taihe::string_view effectId)
{
    CALL_LOG_ENTER;
    bool isSupportEffect = false;
    int32_t ret = IsSupportEffect(effectId.c_str(), &isSupportEffect);
    if (ret != SUCCESS) {
        taihe::set_business_error(ret, "IsSupportEffect execution failed");
    }
    return isSupportEffect;
}
}  // namespace

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_stopVibrationByModeSync(stopVibrationByModeSync);
TH_EXPORT_CPP_API_startVibrationSync(startVibrationSync);
TH_EXPORT_CPP_API_isHdHapticSupported(isHdHapticSupported);
TH_EXPORT_CPP_API_stopVibrationSync(stopVibrationSync);
TH_EXPORT_CPP_API_isSupportEffectSync(isSupportEffectSync);
// NOLINTEND
