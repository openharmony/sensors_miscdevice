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

#include <array>
#include <set>
#include <iostream>
#include "napi/native_api.h"
#include "ani_utils.h"
#include "file_utils.h"
#include <map>
#include "miscdevice_log.h"
#include "vibrator_agent.h"

#undef LOG_TAG
#define LOG_TAG "VibratorAni"

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

typedef struct VibrateInfo {
    std::string type;
    std::string usage;
    bool systemUsage;
    int32_t duration = 0;
    std::string effectId;
    int32_t count = 0;
    int32_t fd = -1;
    int64_t offset = 0;
    int64_t length = -1;
    int32_t intensity = 0;
    VibratorPattern vibratorPattern;
} VibrateInfo;

static void ThrowBusinessError(ani_env *env, int errCode, std::string&& errMsg)
{
    MISC_HILOGD("Begin ThrowBusinessError.");
    static const char *errorClsName = "L@ohos/base/BusinessError;";
    ani_class cls {};
    if (ANI_OK != env->FindClass(errorClsName, &cls)) {
        MISC_HILOGE("find class BusinessError %{public}s failed", errorClsName);
        return;
    }
    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", ":V", &ctor)) {
        MISC_HILOGE("find method BusinessError.constructor failed");
        return;
    }
    ani_object errorObject;
    if (ANI_OK != env->Object_New(cls, ctor, &errorObject)) {
        MISC_HILOGE("create BusinessError object failed");
        return;
    }
    ani_double aniErrCode = static_cast<ani_double>(errCode);
    ani_string errMsgStr;
    if (ANI_OK != env->String_NewUTF8(errMsg.c_str(), errMsg.size(), &errMsgStr)) {
        MISC_HILOGE("convert errMsg to ani_string failed");
        return;
    }
    if (ANI_OK != env->Object_SetFieldByName_Double(errorObject, "code", aniErrCode)) {
        MISC_HILOGE("set error code failed");
        return;
    }
    if (ANI_OK != env->Object_SetPropertyByName_Ref(errorObject, "message", errMsgStr)) {
        MISC_HILOGE("set error message failed");
        return;
    }
    env->ThrowError(static_cast<ani_error>(errorObject));
    return;
}

static bool ParserParamFromVibrateTime(ani_env *env, ani_object effect, VibrateInfo &vibrateInfo)
{
    ani_ref type;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(effect, "type", &type)) {
        MISC_HILOGE("Failed to get property named type");
        return false;
    }
    auto typeStr = AniStringUtils::ToStd(env, static_cast<ani_string>(type));
    vibrateInfo.type = typeStr;
    MISC_HILOGD("vibrateInfo.type: %{public}s", typeStr.c_str());

    ani_double duration;
    if (ANI_OK != env->Object_GetPropertyByName_Double(effect, "duration", &duration)) {
        MISC_HILOGE("Failed to get property named duration");
        return false;
    }
    vibrateInfo.duration = static_cast<double>(duration);
    MISC_HILOGD("vibrateInfo.duration: %{public}d", vibrateInfo.duration);
    return true;
}

static bool SetVibrateProperty(ani_env* env, ani_object effect, const char* propertyName, int32_t& propertyValue)
{
    ani_ref propertyRef;
    ani_boolean isUndefined = false;
    if (env->Object_GetPropertyByName_Ref(effect, propertyName, &propertyRef) != ANI_OK) {
        MISC_HILOGD("Can not find \"%{public}s\" property", propertyName);
        return false;
    }

    env->Reference_IsUndefined(propertyRef, &isUndefined);
    if (isUndefined) {
        MISC_HILOGD("\"%{public}s\" is undefined", propertyName);
        return false;
    }

    ani_double result;
    if (ANI_OK != env->Object_CallMethodByName_Double(static_cast<ani_object>(propertyRef), "doubleValue", nullptr,
        &result)) {
        MISC_HILOGE("Failed to call Method named doubleValue on property \"%{public}s\"", propertyName);
        return false;
    }

    propertyValue = static_cast<double>(result);
    MISC_HILOGD("\"%{public}s\": %{public}d", propertyName, propertyValue);

    return true;
}

static bool ParserParamFromVibratePreset(ani_env *env, ani_object effect, VibrateInfo &vibrateInfo)
{
    ani_ref type;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(effect, "type", &type)) {
        MISC_HILOGE("Failed to get property named type");
        return false;
    }
    auto typeStr = AniStringUtils::ToStd(env, static_cast<ani_string>(type));
    vibrateInfo.type = typeStr;
    MISC_HILOGD("vibrateInfo.type: %{public}s", typeStr.c_str());

    ani_ref effectId;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(effect, "effectId", &effectId)) {
        MISC_HILOGE("Failed to get property named effectId");
        return false;
    }
    auto effectIdStr = AniStringUtils::ToStd(env, static_cast<ani_string>(effectId));
    vibrateInfo.effectId = effectIdStr;
    MISC_HILOGD("effectId: %{public}s", vibrateInfo.effectId.c_str());

    if (!SetVibrateProperty(env, effect, "count", vibrateInfo.count)) {
        vibrateInfo.count = 1;
    }
    MISC_HILOGD("count: %{public}d", vibrateInfo.count);

    if (!SetVibrateProperty(env, effect, "intensity", vibrateInfo.intensity)) {
        vibrateInfo.intensity = INTENSITY_ADJUST_MAX;
    }
    MISC_HILOGD("intensity: %{public}d", vibrateInfo.intensity);
    return true;
}

static bool SetVibratePropertyInt64(ani_env* env, ani_object effect, const char* propertyName, int64_t& propertyValue)
{
    ani_ref propertyRef;
    ani_boolean isUndefined = false;
    if (env->Object_GetPropertyByName_Ref(effect, propertyName, &propertyRef) != ANI_OK) {
        MISC_HILOGD("Can not find \"%{public}s\" property", propertyName);
        return false;
    }

    env->Reference_IsUndefined(propertyRef, &isUndefined);
    if (isUndefined) {
        MISC_HILOGD("\"%{public}s\" is undefined", propertyName);
        return false;
    }

    ani_double result;
    if (ANI_OK != env->Object_CallMethodByName_Double(static_cast<ani_object>(propertyRef), "doubleValue", nullptr,
        &result)) {
        MISC_HILOGE("Failed to call Method named doubleValue on property \"%{public}s\"", propertyName);
        return false;
    }

    propertyValue = static_cast<double>(result);
    MISC_HILOGD("\"%{public}s\": %{public}lld", propertyName, propertyValue);

    return true;
}

static bool ParserParamFromVibrateFromFile(ani_env *env, ani_object effect, VibrateInfo &vibrateInfo)
{
    ani_ref type;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(effect, "type", &type)) {
        MISC_HILOGE("Failed to get property named type");
        return false;
    }
    auto typeStr = AniStringUtils::ToStd(env, static_cast<ani_string>(type));
    vibrateInfo.type = typeStr;

    ani_ref hapticFd;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(effect, "hapticFd", &hapticFd)) {
        MISC_HILOGE("Failed to get property named hapticFd");
        return false;
    }

    ani_double fd;
    if (ANI_OK != env->Object_GetPropertyByName_Double(static_cast<ani_object>(hapticFd), "fd", &fd)) {
        MISC_HILOGE("Failed to get property named fd");
        return false;
    }
    vibrateInfo.fd = static_cast<double>(fd);
    MISC_HILOGD("vibrateInfo.type: %{public}s, vibrateInfo.fd: %{public}d", typeStr.c_str(), vibrateInfo.fd);

    SetVibratePropertyInt64(env, static_cast<ani_object>(hapticFd), "offset", vibrateInfo.offset);
    MISC_HILOGD("vibrateInfo.offset: %{public}lld", vibrateInfo.offset);
    int64_t fdSize = GetFileSize(vibrateInfo.fd);
    if (!(vibrateInfo.offset >= 0) && (vibrateInfo.offset <= fdSize)) {
        MISC_HILOGE("The parameter of offset is invalid");
        return false;
    }
    vibrateInfo.length = fdSize - vibrateInfo.offset;
    SetVibratePropertyInt64(env, static_cast<ani_object>(hapticFd), "length", vibrateInfo.length);
    MISC_HILOGD("vibrateInfo.length: %{public}lld", vibrateInfo.length);
    return true;
}

static bool SetVibrateBooleanProperty(ani_env* env, ani_object attribute, const char* propertyName,
    VibrateInfo& vibrateInfo)
{
    ani_ref propertyRef;
    ani_boolean isUndefined = false;
    if (env->Object_GetPropertyByName_Ref(attribute, propertyName, &propertyRef) != ANI_OK) {
        MISC_HILOGE("Failed to get \"%{public}s\" property", propertyName);
        return false;
    }

    env->Reference_IsUndefined(propertyRef, &isUndefined);
    if (isUndefined) {
        MISC_HILOGE("\"%{public}s\" is not undefined", propertyName);
        return false;
    }

    ani_boolean result;
    if (ANI_OK != env->Object_CallMethodByName_Boolean(static_cast<ani_object>(propertyRef), "unboxed", nullptr,
        &result)) {
        MISC_HILOGE("Failed to call Method named unboxed on property \"%{public}s\"", propertyName);
        return false;
    }

    vibrateInfo.systemUsage = static_cast<bool>(result);
    return true;
}

static bool ParserParamFromVibrateAttribute(ani_env *env, ani_object attribute, VibrateInfo &vibrateInfo)
{
    ani_ref usage;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(attribute, "usage", &usage)) {
        MISC_HILOGE("Object_GetPropertyByName_Ref Fail");
        return false;
    }
    auto usageStr = AniStringUtils::ToStd(env, static_cast<ani_string>(usage));
    vibrateInfo.usage = usageStr;
    MISC_HILOGD("vibrateInfo.usage: %{public}s", vibrateInfo.usage.c_str());

    if (!SetVibrateBooleanProperty(env, attribute, "systemUsage", vibrateInfo)) {
        vibrateInfo.systemUsage = false;
    }
    MISC_HILOGD("vibrateInfo.systemUsage: %{public}d", vibrateInfo.systemUsage);
    return true;
}

static bool GetPropertyAsDouble(ani_env* env, ani_object obj, const char* propertyName, ani_double* outValue)
{
    ani_ref propRef = nullptr;
    ani_boolean isUndefined = false;

    if (env->Object_GetPropertyByName_Ref(obj, propertyName, &propRef) != ANI_OK) {
        MISC_HILOGE("GetPropertyAsDouble: Failed to get property '%{punlic}s'", propertyName);
        return false;
    }

    env->Reference_IsUndefined(propRef, &isUndefined);
    if (isUndefined) {
        MISC_HILOGE("GetPropertyAsDouble: Property '%{punlic}s' is undefined", propertyName);
        return false;
    }

    if (env->Object_CallMethodByName_Double(static_cast<ani_object>(propRef), "doubleValue", nullptr, outValue) !=
        ANI_OK) {
        MISC_HILOGE("GetPropertyAsDouble: Failed to call 'doubleValue' on property '%{punlic}s'", propertyName);
        return false;
    }

    return true;
}

static bool ParseVibratorCurvePoint(ani_env *env, ani_object pointsArray, uint32_t index, VibratorCurvePoint &point)
{
    auto array = static_cast<ani_array_ref>(pointsArray);
    ani_ref arrayRef;
    if (env->Array_Get_Ref(array, index, &arrayRef) != ANI_OK) {
        MISC_HILOGE("Array_Get_Ref Fail");
        return false;
    }
    ani_double time = 0;
    if (ANI_OK != env->Object_GetPropertyByName_Double(static_cast<ani_object>(arrayRef), "time", &time)) {
        MISC_HILOGE("Object_GetPropertyByName_Double Fail");
        return false;
    }
    point.time = static_cast<int32_t>(time);
    MISC_HILOGD("ParseVibrateEvent point.time: %{public}d", point.time);

    ani_double intensity = 0;
    ani_double frequency = 0;
    if (GetPropertyAsDouble(env, static_cast<ani_object>(arrayRef), "intensity", &intensity)) {
        point.intensity = static_cast<int32_t>(intensity);
        MISC_HILOGD("ParseVibrateEvent point.intensity: %{public}d", point.intensity);
    }

    if (GetPropertyAsDouble(env, static_cast<ani_object>(arrayRef), "frequency", &frequency)) {
        point.frequency = static_cast<int32_t>(frequency);
        MISC_HILOGD("ParseVibrateEvent point.frequency: %{public}d", point.frequency);
    }

    return true;
}

static bool ParseVibratorCurvePointArray(ani_env *env, ani_object pointsArray, uint32_t pointsLength,
    VibratorEvent &event)
{
    if (pointsLength <= 0 || pointsLength > CURVE_POINT_NUM_MAX) {
        MISC_HILOGE("pointsLength should not be less than or equal to 0 or greater than CURVE_POINT_NUM_MAX");
        return false;
    }
    VibratorCurvePoint *points =
        static_cast<VibratorCurvePoint *>(malloc(sizeof(VibratorCurvePoint) * pointsLength));
    if (points == nullptr) {
        MISC_HILOGE("malloc failed");
        return false;
    }
    for (uint32_t i = 0; i < pointsLength; ++i) {
        if (!ParseVibratorCurvePoint(env, pointsArray, i, points[i])) {
            MISC_HILOGE("ParseVibratorCurvePoint failed");
            free(points);
            points = nullptr;
            return false;
        }
    }
    event.points = points;
    return true;
}

static bool ParsePointsArray(ani_env *env, ani_object parentObject, VibratorEvent &event)
{
    ani_ref points = nullptr;
    ani_boolean isUndefined = false;
    if (env->Object_GetPropertyByName_Ref(parentObject, "points", &points) != ANI_OK) {
        MISC_HILOGE("Can not find \"points\" property");
        return false;
    }

    env->Reference_IsUndefined(points, &isUndefined);
    if (isUndefined) {
        MISC_HILOGE("\"points\" is undefined");
        return true;
    }

    ani_size sizePoints = 0;
    if (env->Array_GetLength(static_cast<ani_array>(points), &sizePoints) != ANI_OK) {
        MISC_HILOGE("Get point array length failed");
        return false;
    }

    event.pointNum = static_cast<uint32_t>(sizePoints);

    if (static_cast<uint32_t>(sizePoints) > 0) {
        if (!ParseVibratorCurvePointArray(env, static_cast<ani_object>(points),
            static_cast<uint32_t>(sizePoints), event)) {
            MISC_HILOGE("ParseVibratorCurvePointArray failed");
            return false;
        }
    }

    return true;
}

static bool ParseVibrateEvent(ani_env *env, ani_object eventArray, int32_t index, VibratorEvent &event)
{
    auto array = static_cast<ani_array_ref>(eventArray);
    ani_ref arrayRef;
    if (env->Array_Get_Ref(array, index, &arrayRef) != ANI_OK) {
        MISC_HILOGE("Array_Get_Ref Fail");
        return false;
    }
    ani_ref aniEventType;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(static_cast<ani_object>(arrayRef), "eventType", &aniEventType)) {
        MISC_HILOGE("Can not find \"eventType\" property");
        return false;
    }

    EnumAccessor eventTypeAccessor(env, static_cast<ani_enum_item>(aniEventType));
    expected<VibratorEventType, ani_status> eventTypeExpected = eventTypeAccessor.To<VibratorEventType>();
    if (!eventTypeExpected) {
        return false;
    }
    VibratorEventType eventType = eventTypeExpected.value();
    event.type = eventType;

    ani_double time;
    if (ANI_OK != env->Object_GetPropertyByName_Double(static_cast<ani_object>(arrayRef), "time", &time)) {
        MISC_HILOGE("Failed to get property named time");
        return false;
    }
    event.time = static_cast<double>(time);

    ani_double duration;
    if (GetPropertyAsDouble(env, static_cast<ani_object>(arrayRef), "duration", &duration)) {
        event.duration = static_cast<int32_t>(duration);
    }

    ani_double intensity;
    if (GetPropertyAsDouble(env, static_cast<ani_object>(arrayRef), "intensity", &intensity)) {
        event.intensity = static_cast<int32_t>(intensity);
        MISC_HILOGD("intensity: %{public}d", event.intensity);
    }

    ani_double frequency;
    if (GetPropertyAsDouble(env, static_cast<ani_object>(arrayRef), "frequency", &frequency)) {
        event.frequency = static_cast<int32_t>(frequency);
        MISC_HILOGD("frequency: %{public}d", event.frequency);
    }

    ani_double aniIndex;
    if (GetPropertyAsDouble(env, static_cast<ani_object>(arrayRef), "index", &aniIndex)) {
        event.index = static_cast<int32_t>(aniIndex);
        MISC_HILOGD("index: %{public}d", event.index);
    }

    if (!ParsePointsArray(env, static_cast<ani_object>(arrayRef), event)) {
        return false;
    }

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

static bool ParseVibratorPatternEvents(ani_env *env, ani_object patternObj, VibratorPattern &pattern)
{
    ani_ref eventArray;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(static_cast<ani_object>(patternObj), "events", &eventArray)) {
        MISC_HILOGE("Failed to get property named events");
        return false;
    }

    ani_size size;
    if (ANI_OK != env->Array_GetLength(static_cast<ani_array>(eventArray), &size)) {
        MISC_HILOGE("Array_GetLength failed");
        return false;
    }

    pattern.eventNum = static_cast<int32_t>(size);
    MISC_HILOGD("ProcessVibratorPattern eventNum: %{public}d", pattern.eventNum);

    if (size <= 0 || size > EVENT_NUM_MAX) {
        MISC_HILOGE("length should not be less than or equal to 0 or greater than EVENT_NUM_MAX");
        return false;
    }
    VibratorEvent *eventsAlloc =
        static_cast<VibratorEvent *>(malloc(sizeof(VibratorEvent) * static_cast<double>(size)));
    if (eventsAlloc == nullptr) {
        MISC_HILOGE("Events is nullptr");
        return false;
    }

    for (uint32_t j = 0; j < size; ++j) {
        new (&eventsAlloc[j]) VibratorEvent();
        if (!ParseVibrateEvent(env, static_cast<ani_object>(eventArray), j, eventsAlloc[j])) {
            MISC_HILOGE("ParseVibrateEvent failed");
            free(eventsAlloc);
            eventsAlloc = nullptr;
            return false;
        }
    }
    pattern.events = eventsAlloc;
    return true;
}

static bool ParserParamFromVibratePattern(ani_env *env, ani_object effect, VibrateInfo &vibrateInfo)
{
    MISC_HILOGD("ParserParamFromVibratePattern enter");
    ani_ref type;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(effect, "type", &type)) {
        MISC_HILOGE("Failed to get property named type");
        return false;
    }
    auto typeStr = AniStringUtils::ToStd(env, static_cast<ani_string>(type));
    vibrateInfo.type = typeStr;

    ani_ref pattern;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(effect, "pattern", &pattern)) {
        MISC_HILOGE("Failed to get property named pattern");
        return false;
    }

    ani_double time;
    if (ANI_OK != env->Object_GetPropertyByName_Double(static_cast<ani_object>(pattern), "time", &time)) {
        MISC_HILOGE("Failed to get property named time");
        return false;
    }

    vibrateInfo.vibratorPattern.time = static_cast<int32_t>(time);
    MISC_HILOGD("ProcessVibratorPattern time: %{public}d", vibrateInfo.vibratorPattern.time);

    if (!ParseVibratorPatternEvents(env, static_cast<ani_object>(pattern), vibrateInfo.vibratorPattern)) {
        return false;
    }

    PrintVibratorPattern(vibrateInfo.vibratorPattern);

    if (!CheckVibratorPatternParameter(vibrateInfo.vibratorPattern)) {
        MISC_HILOGE("CheckVibratorPatternParameter fail");
        return false;
    }
    return true;
}

bool SetUsage(const std::string &usage, bool systemUsage)
{
    if (auto iter = g_usageType.find(usage); iter == g_usageType.end()) {
        MISC_HILOGE("Wrong usage type");
        return false;
    }
    return SetUsage(g_usageType[usage], systemUsage);
}

static int32_t StartVibrate(const VibrateInfo &info)
{
    if (!SetUsage(info.usage, info.systemUsage)) {
        MISC_HILOGE("SetUsage fail");
        return PARAMETER_ERROR;
    }
    if (g_allowedTypes.find(info.type) == g_allowedTypes.end()) {
        MISC_HILOGE("Invalid vibrate type, type:%{public}s", info.type.c_str());
        return PARAMETER_ERROR;
    }
    if (info.type == "preset") {
        if (!SetLoopCount(info.count)) {
            MISC_HILOGE("SetLoopCount fail");
            return PARAMETER_ERROR;
        }
        return PlayPrimitiveEffect(info.effectId.c_str(), info.intensity);
    } else if (info.type == "file") {
        return PlayVibratorCustom(info.fd, info.offset, info.length);
    } else if (info.type == "pattern") {
        return PlayPattern(info.vibratorPattern);
    }
    return StartVibratorOnce(info.duration);
}

static ani_class FindClassInNamespace(ani_env *env, ani_namespace &ns, const char *className)
{
    ani_class cls;
    if (ANI_OK != env->Namespace_FindClass(ns, className, &cls)) {
        MISC_HILOGE("Not found '%{public}s'", className);
        return nullptr;
    }
    return cls;
}

static bool ParseEffectTypeAndParameters(ani_env *env, ani_object effect, VibrateInfo &vibrateInfo)
{
    ani_namespace ns;
    static const char *namespaceName = "L@ohos/vibrator/vibrator;";
    if (ANI_OK != env->FindNamespace(namespaceName, &ns)) {
        MISC_HILOGE("Not found '%{public}s'", namespaceName);
        return false;
    }

    ani_class vibrateTimeClass = FindClassInNamespace(env, ns, "LVibrateTime;");
    ani_class vibratePresetClass = FindClassInNamespace(env, ns, "LVibratePreset;");
    ani_class vibrateFromFileClass = FindClassInNamespace(env, ns, "LVibrateFromFile;");
    ani_class vibratePatternClass = FindClassInNamespace(env, ns, "LVibrateFromPattern;");
    if (!vibrateTimeClass || !vibratePresetClass || !vibrateFromFileClass || !vibratePatternClass) {
        return false;
    }

    ani_boolean isInstanceOfTime = ANI_FALSE;
    env->Object_InstanceOf(effect, vibrateTimeClass, &isInstanceOfTime);
    ani_boolean isInstanceOfPreset = ANI_FALSE;
    env->Object_InstanceOf(effect, vibratePresetClass, &isInstanceOfPreset);
    ani_boolean isInstanceOfFile = ANI_FALSE;
    env->Object_InstanceOf(effect, vibrateFromFileClass, &isInstanceOfFile);
    ani_boolean isInstanceOfPattern = ANI_FALSE;
    env->Object_InstanceOf(effect, vibratePatternClass, &isInstanceOfPattern);

    if (isInstanceOfTime) {
        if (!ParserParamFromVibrateTime(env, effect, vibrateInfo)) {
            ThrowBusinessError(env, PARAMETER_ERROR, "ParserParamFromVibrateTime failed!");
            return false;
        }
    } else if (isInstanceOfPreset) {
        if (!ParserParamFromVibratePreset(env, effect, vibrateInfo)) {
            ThrowBusinessError(env, PARAMETER_ERROR, "ParserParamFromVibratePreset failed!");
            return false;
        }
    } else if (isInstanceOfFile) {
        if (!ParserParamFromVibrateFromFile(env, effect, vibrateInfo)) {
            ThrowBusinessError(env, PARAMETER_ERROR, "ParserParamFromVibrateFromFile failed!");
            return false;
        }
    } else if (isInstanceOfPattern) {
        if (!ParserParamFromVibratePattern(env, effect, vibrateInfo)) {
            ThrowBusinessError(env, PARAMETER_ERROR, "ParserParamFromVibratePattern failed!");
            return false;
        }
    } else {
        ThrowBusinessError(env, PARAMETER_ERROR, "Unknown effect type");
        return false;
    }

    return true;
}

static void StartVibrationSync([[maybe_unused]] ani_env *env, ani_object effect, ani_object attribute)
{
    ani_namespace ns;
    static const char *namespaceName = "L@ohos/vibrator/vibrator;";
    if (ANI_OK != env->FindNamespace(namespaceName, &ns)) {
        MISC_HILOGE("Not found '%{public}s'", namespaceName);
        return;
    }

    ani_class vibrateAttributeClass = FindClassInNamespace(env, ns, "LVibrateAttribute;");
    if (!vibrateAttributeClass) {
        return;
    }

    VibrateInfo vibrateInfo;
    if (!ParseEffectTypeAndParameters(env, effect, vibrateInfo)) {
        return;
    }
    if (!ParserParamFromVibrateAttribute(env, attribute, vibrateInfo)) {
        ThrowBusinessError(env, PARAMETER_ERROR, "ParserParamFromVibrateAttribute failed!");
        return;
    }
    StartVibrate(vibrateInfo);
}

static bool IsSupportEffectInterally([[maybe_unused]] ani_env *env, ani_string effectId)
{
    auto effectIdStr = AniStringUtils::ToStd(env, static_cast<ani_string>(effectId));
    MISC_HILOGD("effectId:%{public}s", effectIdStr.c_str());

    bool isSupportEffect = false;
    if (IsSupportEffect(effectIdStr.c_str(), &isSupportEffect) != 0) {
        MISC_HILOGE("Query effect support failed");
    }
    return isSupportEffect;
}

ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
    ani_env *env;
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &env)) {
        MISC_HILOGE("Unsupported ANI_VERSION_1");
        return ANI_ERROR;
    }

    static const char *namespaceName = "L@ohos/vibrator/vibrator;";
    ani_namespace ns;
    if (ANI_OK != env->FindNamespace(namespaceName, &ns)) {
        MISC_HILOGE("Not found '%{public}s'", namespaceName);
        return ANI_NOT_FOUND;
    }

    std::array methods = {
        ani_native_function {"startVibrationSync", nullptr, reinterpret_cast<void *>(StartVibrationSync)},
        ani_native_function {"isSupportEffectInterally", nullptr, reinterpret_cast<void *>(IsSupportEffectInterally)},
    };

    if (ANI_OK != env->Namespace_BindNativeFunctions(ns, methods.data(), methods.size())) {
        MISC_HILOGE("Cannot bind native methods to '%{public}s'", namespaceName);
        return ANI_NOT_FOUND;
    };

    *result = ANI_VERSION_1;
    return ANI_OK;
}