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

#include "refbase.h"
#include "file_utils.h"
#include "miscdevice_log.h"
#include "sensors_errors.h"
#include "vibrator_agent.h"

#undef LOG_TAG
#define LOG_TAG "VibratorImpl"

namespace {
using namespace taihe;
using namespace OHOS::Sensors;
// using namespace ohos::vibrator;
using namespace OHOS;

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
constexpr int32_t DEFAULT_EVENT_INTENSITY = 100;
constexpr int32_t DEFAULT_EVENT_FREQUENCY = 50;
constexpr int32_t TRANSIENT_VIBRATION_DURATION = 48;

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

enum FunctionType {
    SYSTEM_VIBRATE_CALLBACK = 1,
    COMMON_CALLBACK,
    IS_SUPPORT_EFFECT_CALLBACK,
    VIBRATOR_STATE_CHANGE,
};

typedef struct AniCallbackData {
    bool isSupportEffect;
    std::string flag;
    FunctionType funcType;
} AniCallbackData;

static std::set<std::string> g_allowedTypes = {"time", "preset", "file", "pattern"};
using callbackType = std::variant<taihe::callback<void(::ohos::vibrator::VibratorStatusEvent const&)>>;

struct CallbackObject : public RefBase {
    CallbackObject(callbackType cb, ani_ref ref) : callback(cb), ref(ref)
    {
    }
    ~CallbackObject()
    {
    }
    callbackType callback;
    ani_ref ref;
};

void CallBackVibratorStatusEvent(::ohos::vibrator::VibratorStatusEvent event, sptr<CallbackObject> callbackObject);

struct AsyncCallbackError {
    int32_t code {0};
    std::string message;
    std::string name;
    std::string stack;
};

static std::map<std::string, std::vector<sptr<CallbackObject>>> g_onCallbackInfos;
static std::mutex g_Mutex;

std::map<std::string, std::function<void(::ohos::vibrator::VibratorStatusEvent, sptr<CallbackObject>)>>
    g_functionByVibratorStatusEvent = {
        { "vibratorStateChange", CallBackVibratorStatusEvent },
    };

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
    int32_t ret = StopVibrator(stopMode.get_value());
    if (ret != SUCCESS) {
        taihe::set_business_error(ret, "stop vibrator by mode failed");
        return;
    }
}

static bool ParseVibratorCurvePoint(::taihe::array<::ohos::vibrator::VibratorCurvePoint> pointsArray, uint32_t index,
    VibratorCurvePoint &point)
{
    CALL_LOG_ENTER;
    point.time = pointsArray[index].time;
    point.intensity =
        pointsArray[index].intensity.has_value() ? static_cast<int32_t>(pointsArray[index].intensity.value()) : 0;
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

static void PrintVibratorPattern(VibratorPattern &vibratorPattern)
{
    CALL_LOG_ENTER;
    if (vibratorPattern.events == nullptr) {
        MISC_HILOGE("Events is nullptr");
        return;
    }
    MISC_HILOGD("PrintVibratorPattern, time:%{public}d, eventNum:%{public}d",
        vibratorPattern.time, vibratorPattern.eventNum);
    for (int32_t i = 0; i < vibratorPattern.eventNum; ++i) {
        MISC_HILOGD("PrintVibratorPattern, type:%{public}d, time:%{public}d, duration:%{public}d, \
            intensity:%{public}d, frequency:%{public}d, index:%{public}d, pointNum:%{public}d",
            static_cast<int32_t>(vibratorPattern.events[i].type), vibratorPattern.events[i].time,
            vibratorPattern.events[i].duration, vibratorPattern.events[i].intensity,
            vibratorPattern.events[i].frequency, vibratorPattern.events[i].index, vibratorPattern.events[i].pointNum);
        if (vibratorPattern.events[i].pointNum > 0) {
            VibratorCurvePoint *point = vibratorPattern.events[i].points;
            for (int32_t j = 0; j < vibratorPattern.events[i].pointNum; ++j) {
                MISC_HILOGD("PrintVibratorPattern, time:%{public}d, intensity:%{public}d, frequency:%{public}d",
                    point[j].time, point[j].intensity, point[j].frequency);
            }
        }
    }
}

static bool CheckVibratorCurvePoint(const VibratorEvent &event)
{
    CALL_LOG_ENTER;
    if ((event.pointNum < CURVE_POINT_NUM_MIN) || (event.pointNum > CURVE_POINT_NUM_MAX)) {
        MISC_HILOGE("The points size is out of range, pointNum:%{public}d", event.pointNum);
        return false;
    }
    for (int32_t j = 0; j < event.pointNum; ++j) {
        if ((event.points[j].time < 0) || (event.points[j].time > event.duration)) {
            MISC_HILOGE("time in points is out of range, time:%{public}d", event.points[j].time);
            return false;
        }
        if ((event.points[j].intensity < 0) || (event.points[j].intensity > CURVE_POINT_INTENSITY_MAX)) {
            MISC_HILOGE("intensity in points is out of range, intensity:%{public}d", event.points[j].intensity);
            return false;
        }
        if ((event.points[j].frequency < CURVE_FREQUENCY_MIN) || (event.points[j].frequency > CURVE_FREQUENCY_MAX)) {
            MISC_HILOGE("frequency in points is out of range, frequency:%{public}d", event.points[j].frequency);
            return false;
        }
    }
    return true;
}

static bool CheckVibratorEvent(const VibratorEvent &event)
{
    CALL_LOG_ENTER;
    if ((event.time < 0) || (event.time > EVENT_START_TIME_MAX)) {
        MISC_HILOGE("The event time is out of range, time:%{public}d", event.time);
        return false;
    }
    if ((event.frequency < FREQUENCY_MIN) || (event.frequency > FREQUENCY_MAX)) {
        MISC_HILOGE("The event frequency is out of range, frequency:%{public}d", event.frequency);
        return false;
    }
    if ((event.intensity < INTENSITY_MIN) || (event.intensity > INTENSITY_MAX)) {
        MISC_HILOGE("The event intensity is out of range, intensity:%{public}d", event.intensity);
        return false;
    }
    if ((event.duration <= 0) || (event.duration > CONTINUOUS_DURATION_MAX)) {
        MISC_HILOGE("The event duration is out of range, duration:%{public}d", event.duration);
        return false;
    }
    if ((event.index < 0) || (event.index > EVENT_INDEX_MAX)) {
        MISC_HILOGE("The event index is out of range, index:%{public}d", event.index);
        return false;
    }
    if ((event.type == VibratorEventType::EVENT_TYPE_CONTINUOUS) && (event.pointNum > 0)) {
        if (!CheckVibratorCurvePoint(event)) {
            MISC_HILOGE("CheckVibratorCurvePoint failed");
            return false;
        }
    }
    return true;
}

static bool CheckVibratorPatternParameter(VibratorPattern &vibratorPattern)
{
    CALL_LOG_ENTER;
    if (vibratorPattern.events == nullptr) {
        MISC_HILOGE("Events is nullptr");
        return false;
    }
    if ((vibratorPattern.eventNum <= 0) || (vibratorPattern.eventNum > EVENT_NUM_MAX)) {
        MISC_HILOGE("The event num  is out of range, eventNum:%{public}d", vibratorPattern.eventNum);
        return false;
    }
    for (int32_t i = 0; i < vibratorPattern.eventNum; ++i) {
        if (!CheckVibratorEvent(vibratorPattern.events[i])) {
            MISC_HILOGE("CheckVibratorEvent failed");
            return false;
        }
    }
    return true;
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
    vibratorPattern.events = nullptr;
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
        PrintVibratorPattern(vibrateInfo.vibratorPattern);
        if (!CheckVibratorPatternParameter(vibrateInfo.vibratorPattern)) {
            MISC_HILOGE("CheckVibratorPatternParameter fail");
            ClearVibratorPattern(vibrateInfo.vibratorPattern);
            return false;
        }
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
            taihe::set_business_error(ret, "start vibrator failed");
            return;
        }
    } else {
        int32_t ret = StartVibrate(vibrateInfo);
        if (vibrateInfo.vibratorPattern.events != nullptr) {
            CHKCV(ClearVibratorPattern(vibrateInfo.vibratorPattern), "ClearVibratorPattern fail");
        }
        if (ret != SUCCESS) {
            taihe::set_business_error(ret, "start vibrator failed");
            return;
        }
    }
}

bool isHdHapticSupported()
{
    CALL_LOG_ENTER;
    return IsHdHapticSupported();
}

taihe::array<ohos::vibrator::VibratorInfo> getVibratorInfoSync(
    taihe::optional_view<ohos::vibrator::VibratorInfoParam> param)
{
    VibratorIdentifier identifier;
    if (param.has_value()) {
        const auto &vibratorInfoParam = param.value();
        if (vibratorInfoParam.deviceId.has_value()) {
            identifier.deviceId = vibratorInfoParam.deviceId.value();
        } else {
            MISC_HILOGW("deviceId is undefined, set default value deviceId = -1");
        }
        if (vibratorInfoParam.vibratorId.has_value()) {
            identifier.vibratorId = vibratorInfoParam.vibratorId.value();
        } else {
            MISC_HILOGW("vibratorId is undefined, set default value vibratorId = -1");
        }
        MISC_HILOGD("identifier=[deviceId=%{public}d, vibratorId=%{public}d]",
            identifier.deviceId, identifier.vibratorId);
    } else {
        MISC_HILOGW("deviceId and vibratorId is undefined, set default value deviceId = -1 and vibratorId = -1");
    }
    std::vector<VibratorInfos> vibratorInfo;
    std::vector<ohos::vibrator::VibratorInfo> taiheVibratorInfo;
    int32_t ret = GetVibratorList(identifier, vibratorInfo);
    if (ret == SUCCESS) {
        for (auto& info : vibratorInfo) {
            ohos::vibrator::VibratorInfo taiheInfo = {
                .deviceId = info.deviceId,
                .vibratorId = info.vibratorId,
                .deviceName = taihe::string(info.deviceName),
                .isHdHapticSupported = info.isSupportHdHaptic,
                .isLocalVibrator = info.isLocalVibrator
            };
            taiheVibratorInfo.push_back(taiheInfo);
        }
    }
    return taihe::array<ohos::vibrator::VibratorInfo>(taiheVibratorInfo);
}

void stopVibrationAsync()
{
    CALL_LOG_ENTER;
    int32_t ret = Cancel();
    if (ret != SUCCESS) {
        taihe::set_business_error(ret, "Cancel execution fail");
    }
}

bool isSupportEffectAsync(::taihe::string_view effectId)
{
    CALL_LOG_ENTER;
    bool isSupportEffect = false;
    int32_t ret = IsSupportEffect(effectId.c_str(), &isSupportEffect);
    if (ret != SUCCESS) {
        taihe::set_business_error(ret, "IsSupportEffect execution failed");
    }
    return isSupportEffect;
}

void stopVibrationSync()
{
    int32_t ret = Cancel();
    if (ret != SUCCESS) {
        taihe::set_business_error(ret, "Cancel execution fail");
    }
}

bool isSupportEffectSync(::taihe::string_view effectId)
{
    return isSupportEffectAsync(effectId);
}

void stopVibrationParam(::taihe::optional_view<::ohos::vibrator::VibratorInfoParam> param)
{
    int32_t ret = SUCCESS;
    if (!param.has_value()) {
        ret = Cancel();
    } else {
        VibratorIdentifier info = {
            .deviceId = param->deviceId.value(),
            .vibratorId = param->vibratorId.value()
        };
        ret = CancelEnhanced(info);
    }
    if (ret != SUCCESS) {
        taihe::set_business_error(ret, "Cancel execution fail");
    }
}

::ohos::vibrator::EffectInfo getEffectInfoSync(::taihe::string_view effectId,
    ::taihe::optional_view<::ohos::vibrator::VibratorInfoParam> param)
{
    EffectInfo effectInfo;
    VibratorIdentifier identifierInfo;
    if (param.has_value()) {
        identifierInfo.deviceId = param->deviceId.value();
        identifierInfo.vibratorId = param->vibratorId.value();
    }
    int32_t ret = GetEffectInfo(identifierInfo, std::string(effectId), effectInfo);
    if (ret != OHOS::ERR_OK) {
        effectInfo.isSupportEffect = false;
        MISC_HILOGW("Get effect info failed, ret:%{public}d", ret);
    }
    ::ohos::vibrator::EffectInfo info = {
        .isEffectSupported = effectInfo.isSupportEffect
    };
    return info;
}

void CallBackVibratorStatusEvent(::ohos::vibrator::VibratorStatusEvent event, sptr<CallbackObject> callbackObject)
{
    auto &func = std::get<taihe::callback<void(::ohos::vibrator::VibratorStatusEvent const&)>>(
        callbackObject->callback);
    func(event);
}

static void UpdateCallbackInfos(std::string& vibratorEvent, callbackType callback, uintptr_t opq)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onSubcribeLock(g_Mutex);
    ani_object callbackObj = reinterpret_cast<ani_object>(opq);
    ani_ref callbackRef;
    ani_env *env = taihe::get_env();
    if (env == nullptr || env->GlobalReference_Create(callbackObj, &callbackRef) != ANI_OK) {
        MISC_HILOGW("Failed to create callbackRef, sensorTypeId:%{public}s", vibratorEvent.c_str());
        return;
    }
    std::vector<sptr<CallbackObject>> callbackInfos = g_onCallbackInfos[vibratorEvent];
    bool isSubscribedCallback =
        std::any_of(callbackInfos.begin(), callbackInfos.end(), [env, callbackRef](const CallbackObject *obj) {
            ani_boolean isEqual = false;
            return (env->Reference_StrictEquals(callbackRef, obj->ref, &isEqual) == ANI_OK) && isEqual;
        });
    if (isSubscribedCallback) {
        env->GlobalReference_Delete(callbackRef);
        MISC_HILOGW("Callback is already subscribed, sensorTypeId:%{public}s", vibratorEvent.c_str());
        return;
    }
    sptr<CallbackObject> taiheCallbackInfo = new (std::nothrow) CallbackObject(callback, callbackRef);
    if (taiheCallbackInfo == nullptr) {
        MISC_HILOGW("taiheCallbackInfo is nullptr");
        return;
    }
    callbackInfos.push_back(taiheCallbackInfo);
    g_onCallbackInfos[vibratorEvent] = callbackInfos;
}

void CallbackVibratorStatusEvent(std::string eventType, VibratorStatusEvent statusEvent,
    sptr<CallbackObject> callbackObject)
{
    if (g_functionByVibratorStatusEvent.find(eventType) == g_functionByVibratorStatusEvent.end()) {
        MISC_HILOGW("SensorTypeId not exist, id:%{public}s", eventType.c_str());
        return;
    }
    ::ohos::vibrator::VibratorStatusEvent event {
        .timestamp = statusEvent.timestamp,
        .deviceId = statusEvent.deviceId,
        .vibratorCount = statusEvent.vibratorCnt,
        .isVibratorOnline = statusEvent.type == PLUG_STATE_EVENT_PLUG_IN ? true : false
    };
    g_functionByVibratorStatusEvent[eventType](event, callbackObject);
}

void DataCallbackImpl(VibratorStatusEvent *statusEvent)
{
    CALL_LOG_ENTER;
    if (statusEvent == nullptr) {
        MISC_HILOGW("statusEvent is nullptr");
        return;
    }
    if (statusEvent->type == PLUG_STATE_EVENT_UNKNOWN) {
        MISC_HILOGE("UpdatePlugInfo failed");
        return;
    }
    std::string eventType("vibratorStateChange");
    std::lock_guard<std::mutex> onCallbackLock(g_Mutex);
    auto it = g_onCallbackInfos.find(eventType);
    if (it == g_onCallbackInfos.end()) {
        MISC_HILOGW("not find callback info");
        return;
    }
    for (auto &callbackInfo : it->second) {
        CallbackVibratorStatusEvent(eventType, *statusEvent, callbackInfo);
    }
}

const VibratorUser user = {
    .callback = DataCallbackImpl
};

void onVibratorStateChange(callback_view<void(::ohos::vibrator::VibratorStatusEvent const& event)> f, uintptr_t opq)
{
    int32_t ret = SubscribeVibratorPlug(user);
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "SubscribeVibratorPlug fail");
        return;
    }
    std::string eventType("vibratorStateChange");
    UpdateCallbackInfos(eventType, f, opq);
}

static int32_t RemoveAllCallback(std::string& eventType)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onCancelLock(g_Mutex);
    std::vector<sptr<CallbackObject>>& callbackInfos = g_onCallbackInfos[eventType];
    for (auto iter = callbackInfos.begin(); iter != callbackInfos.end();) {
        CHKPC(*iter);
        if (auto *env = taihe::get_env()) {
            env->GlobalReference_Delete((*iter)->ref);
        }
        iter = callbackInfos.erase(iter);
    }
    if (callbackInfos.empty()) {
        MISC_HILOGD("No subscription to change");
        g_onCallbackInfos.erase(eventType);
        return callbackInfos.size();
    }
    g_onCallbackInfos[eventType] = callbackInfos;
    return callbackInfos.size();
}

static int32_t RemoveCallback(std::string& eventType, uintptr_t opq)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onCallbackLock(g_Mutex);
    ani_object callbackObj = reinterpret_cast<ani_object>(opq);
    ani_ref callbackRef;
    ani_env *env = taihe::get_env();
    if (env == nullptr || env->GlobalReference_Create(callbackObj, &callbackRef) != ANI_OK) {
        MISC_HILOGE("Failed to create callbackRef, sensorTypeId:%{public}s", eventType.c_str());
        return 0;
    }
    std::vector<sptr<CallbackObject>>& callbackInfos = g_onCallbackInfos[eventType];
    for (auto iter = callbackInfos.begin(); iter != callbackInfos.end();) {
        CHKPC(*iter);
        ani_boolean isEqual = false;
        if ((env->Reference_StrictEquals(callbackRef, (*iter)->ref, &isEqual) == ANI_OK) && isEqual) {
            env->GlobalReference_Delete((*iter)->ref);
            iter = callbackInfos.erase(iter);
            MISC_HILOGE("Remove callback success");
            break;
        } else {
            ++iter;
        }
    }
    env->GlobalReference_Delete(callbackRef);
    if (callbackInfos.empty()) {
        MISC_HILOGD("No subscription to change data");
        g_onCallbackInfos.erase(eventType);
        return 0;
    }
    g_onCallbackInfos[eventType] = callbackInfos;
    return callbackInfos.size();
}

void offVibratorStateChange(::taihe::optional_view<uintptr_t> opq)
{
    int32_t subscribeSize = -1;
    std::string eventType = "vibratorStateChange";
    if (opq.has_value()) {
        subscribeSize = RemoveCallback(eventType, opq.value());
    } else {
        subscribeSize = RemoveAllCallback(eventType);
    }
    if (subscribeSize > 0) {
        MISC_HILOGW("There are other client subscribe system js api as well, not need unsubscribe");
        return;
    }

    int32_t ret = UnSubscribeVibratorPlug(user);
    if (ret != OHOS::ERR_OK) {
        MISC_HILOGE("User callback unsubscribe fail, ret:%{public}d", ret);
    }
}

enum VibrateTag {
    EVENT_TAG_UNKNOWN = -1,
    EVENT_TAG_CONTINUOUS = 0,
    EVENT_TAG_TRANSIENT = 1,
};

struct VibrateCurvePoint {
    int32_t time = 0;
    int32_t intensity = 0;
    int32_t frequency = 0;
};

struct VibrateEvent {
    VibrateTag tag;
    int32_t time = 0;
    int32_t duration = 0;
    int32_t intensity = 0;
    int32_t frequency = 0;
    int32_t index = 0;
    std::vector<VibrateCurvePoint> points;
};

class VibratorPatternBuilderImpl {
public:
    VibratorPatternBuilderImpl() {}

    int64_t GetVibratorPatternBuilderImpl()
    {
        return reinterpret_cast<int64_t>(this);
    }

    ohos::vibrator::VibratorPatternBuilder addContinuousEvent(
        int32_t time, int32_t duration, taihe::optional_view<ohos::vibrator::ContinuousParam> options)
    {
        auto vibratorPatternBuilder =
            taihe::make_holder<VibratorPatternBuilderImpl, ohos::vibrator::VibratorPatternBuilder>();
        auto vibratorPatternBuilderImpl =
            reinterpret_cast<VibratorPatternBuilderImpl *>(vibratorPatternBuilder->GetVibratorPatternBuilderImpl());
        if (vibratorPatternBuilderImpl == nullptr) {
            return vibratorPatternBuilder;
        }
        VibrateEvent event;
        event.tag = VibrateTag::EVENT_TAG_CONTINUOUS;
        event.time = time;
        event.duration = duration;
        if (options.has_value()) {
            const auto &continuousParam = options.value();
            if (continuousParam.intensity.has_value()) {
                event.intensity = continuousParam.intensity.value();
            } else {
                event.intensity = DEFAULT_EVENT_INTENSITY;
            }
            if (continuousParam.frequency.has_value()) {
                event.frequency = continuousParam.frequency.value();
            } else {
                event.frequency = DEFAULT_EVENT_FREQUENCY;
            }
            if (continuousParam.index.has_value()) {
                event.index = continuousParam.index.value();
            } else {
                event.index = 0;
            }
        } else {
            event.intensity = DEFAULT_EVENT_INTENSITY;
            event.frequency = DEFAULT_EVENT_FREQUENCY;
            event.index = 0;
        }
        if (!CheckParameters(event)) {
            taihe::set_business_error(PARAMETER_ERROR, "Invalid parameter");
            return vibratorPatternBuilder;
        }
        events_.push_back(event);
        vibratorPatternBuilderImpl->events_ = events_;
        return vibratorPatternBuilder;
    }

    ohos::vibrator::VibratorPatternBuilder addTransientEvent(
        int32_t time, taihe::optional_view<ohos::vibrator::TransientParam> options)
    {
        auto vibratorPatternBuilder =
            taihe::make_holder<VibratorPatternBuilderImpl, ohos::vibrator::VibratorPatternBuilder>();
        auto vibratorPatternBuilderImpl =
            reinterpret_cast<VibratorPatternBuilderImpl *>(vibratorPatternBuilder->GetVibratorPatternBuilderImpl());
        if (vibratorPatternBuilderImpl == nullptr) {
            return vibratorPatternBuilder;
        }
        VibrateEvent event;
        event.tag = VibrateTag::EVENT_TAG_TRANSIENT;
        event.duration = TRANSIENT_VIBRATION_DURATION;
        event.time = time;
        if (options.has_value()) {
            const auto &transientParam = options.value();
            if (transientParam.intensity.has_value()) {
                event.intensity = transientParam.intensity.value();
            } else {
                event.intensity = DEFAULT_EVENT_INTENSITY;
            }
            if (transientParam.frequency.has_value()) {
                event.frequency = transientParam.frequency.value();
            } else {
                event.frequency = DEFAULT_EVENT_FREQUENCY;
            }
            if (transientParam.index.has_value()) {
                event.index = transientParam.index.value();
            } else {
                event.index = 0;
            }
        } else {
            event.intensity = DEFAULT_EVENT_INTENSITY;
            event.frequency = DEFAULT_EVENT_FREQUENCY;
            event.index = 0;
        }
        if (!CheckParameters(event)) {
            taihe::set_business_error(PARAMETER_ERROR, "Invalid parameter");
            return vibratorPatternBuilder;
        }
        events_.push_back(event);
        vibratorPatternBuilderImpl->events_ = events_;
        return vibratorPatternBuilder;
    }

    ohos::vibrator::VibratorPattern build()
    {
        ohos::vibrator::VibratorPattern result{};
        auto eventNum = events_.size();
        if ((eventNum <= 0) || (eventNum > EVENT_NUM_MAX)) {
            taihe::set_business_error(PARAMETER_ERROR, "The number of events exceeds the range");
            return result;
        }
        result.time = 0;
        result.events = ConvertVibrateEvents(events_);
        return result;
    }

private:
    static bool CheckParameters(const VibrateEvent &event)
    {
        if ((event.time < 0) || (event.time > EVENT_START_TIME_MAX)) {
            MISC_HILOGE("The event time is out of range, time:%{public}d", event.time);
            return false;
        }
        if ((event.frequency < FREQUENCY_MIN) || (event.frequency > FREQUENCY_MAX)) {
            MISC_HILOGE("The event frequency is out of range, frequency:%{public}d", event.frequency);
            return false;
        }
        if ((event.intensity < INTENSITY_MIN) || (event.intensity > INTENSITY_MAX)) {
            MISC_HILOGE("The event intensity is out of range, intensity:%{public}d", event.intensity);
            return false;
        }
        if ((event.duration <= 0) || (event.duration > CONTINUOUS_DURATION_MAX)) {
            MISC_HILOGE("The event duration is out of range, duration:%{public}d", event.duration);
            return false;
        }
        if ((event.index < 0) || (event.index > EVENT_INDEX_MAX)) {
            MISC_HILOGE("The event index is out of range, index:%{public}d", event.index);
            return false;
        }
        if ((event.tag == VibrateTag::EVENT_TAG_CONTINUOUS) && !event.points.empty()) {
            if (!CheckCurvePoints(event)) {
                MISC_HILOGE("CheckCurvePoints failed");
                return false;
            }
        }
        return true;
    }

    static bool CheckCurvePoints(const VibrateEvent &event)
    {
        int32_t pointNum = static_cast<int32_t>(event.points.size());
        if ((pointNum < CURVE_POINT_NUM_MIN) || (pointNum > CURVE_POINT_NUM_MAX)) {
            MISC_HILOGE("The points size is out of range, size:%{public}d", pointNum);
            return false;
        }
        for (int32_t i = 0; i < pointNum; ++i) {
            if ((event.points[i].time < 0) || (event.points[i].time > event.duration)) {
                MISC_HILOGE("time in points is out of range, time:%{public}d", event.points[i].time);
                return false;
            }
            if ((event.points[i].intensity < 0) || (event.points[i].intensity > CURVE_POINT_INTENSITY_MAX)) {
                MISC_HILOGE("intensity in points is out of range, intensity:%{public}d", event.points[i].intensity);
                return false;
            }
            if ((event.points[i].frequency < CURVE_FREQUENCY_MIN) ||
                (event.points[i].frequency > CURVE_FREQUENCY_MAX)) {
                MISC_HILOGE("frequency in points is out of range, frequency:%{public}d", event.points[i].frequency);
                return false;
            }
        }
        return true;
    }

    static ohos::vibrator::VibratorEventType ConvertVibratorEventType(const VibrateTag &tag)
    {
        return ohos::vibrator::VibratorEventType::from_value(static_cast<int32_t>(tag));
    }

    static ohos::vibrator::VibratorEvent ConvertVibrateEvent(const VibrateEvent &event)
    {
        ohos::vibrator::VibratorEvent result = {
            .eventType = ConvertVibratorEventType(event.tag),
            .time = event.time,
            .duration = taihe::optional<int32_t>(std::in_place_t{}, event.duration),
            .intensity = taihe::optional<int32_t>(std::in_place_t{}, event.intensity),
            .frequency = taihe::optional<int32_t>(std::in_place_t{}, event.frequency),
            .index = taihe::optional<int32_t>(std::in_place_t{}, event.index),
            .points = taihe::optional<taihe::array<::ohos::vibrator::VibratorCurvePoint>>(
                std::in_place_t{}, ConvertVibrateCurvePoints(event.points))
        };
        return result;
    }

    static taihe::array<ohos::vibrator::VibratorEvent> ConvertVibrateEvents(const std::vector<VibrateEvent> events)
    {
        std::vector<ohos::vibrator::VibratorEvent> vecEvents;
        for (const auto &event : events) {
            vecEvents.push_back(ConvertVibrateEvent(event));
        }
        return taihe::array<ohos::vibrator::VibratorEvent>(vecEvents);
    }

    static ohos::vibrator::VibratorCurvePoint ConvertVibrateCurvePoint(const VibrateCurvePoint &point)
    {
        ohos::vibrator::VibratorCurvePoint result = {
            .time = point.time,
            .intensity = taihe::optional<double>(std::in_place_t{}, point.intensity),
            .frequency = taihe::optional<int32_t>(std::in_place_t{}, point.frequency)
        };
        return result;
    }

    static taihe::array<ohos::vibrator::VibratorCurvePoint> ConvertVibrateCurvePoints(
        const std::vector<VibrateCurvePoint> points)
    {
        std::vector<ohos::vibrator::VibratorCurvePoint> vecPoints;
        for (const auto &point : points) {
            vecPoints.push_back(ConvertVibrateCurvePoint(point));
        }
        return taihe::array<ohos::vibrator::VibratorCurvePoint>(vecPoints);
    }

private:
    std::vector<VibrateEvent> events_;
};

ohos::vibrator::VibratorPatternBuilder getVibratorPatternBuilder()
{
    return taihe::make_holder<VibratorPatternBuilderImpl, ohos::vibrator::VibratorPatternBuilder>();
}
}  // namespace

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_stopVibrationByModeSync(stopVibrationByModeSync);
TH_EXPORT_CPP_API_startVibrationSync(startVibrationSync);
TH_EXPORT_CPP_API_isHdHapticSupported(isHdHapticSupported);
TH_EXPORT_CPP_API_getVibratorInfoSync(getVibratorInfoSync);
TH_EXPORT_CPP_API_stopVibrationAsync(stopVibrationAsync);
TH_EXPORT_CPP_API_isSupportEffectAsync(isSupportEffectAsync);
TH_EXPORT_CPP_API_getVibratorPatternBuilder(getVibratorPatternBuilder);
TH_EXPORT_CPP_API_stopVibrationSync(stopVibrationSync);
TH_EXPORT_CPP_API_isSupportEffectSync(isSupportEffectSync);
TH_EXPORT_CPP_API_stopVibrationParam(stopVibrationParam);
TH_EXPORT_CPP_API_getEffectInfoSync(getEffectInfoSync);
TH_EXPORT_CPP_API_onVibratorStateChange(onVibratorStateChange);
TH_EXPORT_CPP_API_offVibratorStateChange(offVibratorStateChange);
// NOLINTEND
