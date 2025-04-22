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

bool SetVibratePropertyInt64(ani_env* env, ani_object effect, const char* propertyName, int64_t& propertyValue)
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

ani_class FindClassInNamespace(ani_env *env, ani_namespace &ns, const char *className)
{
    ani_class cls;
    if (ANI_OK != env->Namespace_FindClass(ns, className, &cls)) {
        MISC_HILOGE("Not found '%{public}s'", className);
        return nullptr;
    }
    return cls;
}

static void StartVibrationSync([[maybe_unused]] ani_env *env, ani_object effect, ani_object attribute)
{
    ani_namespace ns;
    static const char *namespaceName = "L@ohos/vibrator/vibrator;";
    if (ANI_OK != env->FindNamespace(namespaceName, &ns)) {
        MISC_HILOGE("Not found '%{public}s'", namespaceName);
        return;
    }

    ani_class vibrateTimeClass = FindClassInNamespace(env, ns, "LVibrateTime;");
    ani_class vibratePresetClass = FindClassInNamespace(env, ns, "LVibratePreset;");
    ani_class vibrateFromFileClass = FindClassInNamespace(env, ns, "LVibrateFromFile;");
    ani_class vibrateAttributeClass = FindClassInNamespace(env, ns, "LVibrateAttribute;");
    if (!vibrateTimeClass || !vibratePresetClass || !vibrateFromFileClass || !vibrateAttributeClass) {
        return;
    }

    VibrateInfo vibrateInfo;
    ani_boolean isInstanceOfTime = ANI_FALSE;
    env->Object_InstanceOf(effect, vibrateTimeClass, &isInstanceOfTime);
    ani_boolean isInstanceOfPreset = ANI_FALSE;
    env->Object_InstanceOf(effect, vibratePresetClass, &isInstanceOfPreset);
    ani_boolean isInstanceOfFile = ANI_FALSE;
    env->Object_InstanceOf(effect, vibrateFromFileClass, &isInstanceOfFile);
    if (isInstanceOfTime) {
        if (!ParserParamFromVibrateTime(env, effect, vibrateInfo)) {
            ThrowBusinessError(env, PARAMETER_ERROR, "ParserParamFromVibrateTime failed!");
            return;
        }
    } else if (isInstanceOfPreset) {
        if (!ParserParamFromVibratePreset(env, effect, vibrateInfo)) {
            ThrowBusinessError(env, PARAMETER_ERROR, "ParserParamFromVibratePreset failed!");
            return;
        }
    } else if (isInstanceOfFile) {
        if (!ParserParamFromVibrateFromFile(env, effect, vibrateInfo)) {
            ThrowBusinessError(env, PARAMETER_ERROR, "ParserParamFromVibrateFromFile failed!");
            return;
        }
    }

    if (!ParserParamFromVibrateAttribute(env, attribute, vibrateInfo)) {
        ThrowBusinessError(env, PARAMETER_ERROR, "ParserParamFromVibrateAttribute failed!");
        return;
    }
    StartVibrate(vibrateInfo);
}

bool IsSupportEffectInterally([[maybe_unused]] ani_env *env, ani_string effectId)
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