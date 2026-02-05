/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "vibrator_ffi.h"

#include <map>
#include <set>
#include <string>

#include "file_utils.h"
#include "miscdevice_log.h"
#include "sensors_errors.h"
#include "vibrator_agent.h"

#undef LOG_TAG
#define LOG_TAG "Vibrator-Cj"

namespace OHOS {
namespace Sensors {
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
struct CJVibrateInfo {
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
};

char* MallocCString(const std::string& origin)
{
    if (origin.empty()) {
        return nullptr;
    }
    auto length = origin.length() + 1;
    char* res =  static_cast<char*>(malloc(sizeof(char) * length));
    if (res == nullptr) {
        return nullptr;
    }
    return std::char_traits<char>::copy(res, origin.c_str(), length);
}

bool SetUsage(const std::string &usage, bool systemUsage)
{
    if (auto iter = g_usageType.find(usage); iter == g_usageType.end()) {
        MISC_HILOGE("Wrong usage type");
        return false;
    }
    return SetUsage(g_usageType[usage], systemUsage);
}

int32_t StartVibrate(const CJVibrateInfo &info)
{
    CALL_LOG_ENTER;
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

bool SetUsage(const VibratorIdentifier &identifier, const std::string &usage, bool systemUsage)
{
    if (auto iter = g_usageType.find(usage); iter == g_usageType.end()) {
        MISC_HILOGE("Wrong usage type");
        return false;
    }
    return SetUsageEnhanced(identifier, g_usageType[usage], systemUsage);
}

int32_t CheckParameters(const CJVibrateInfo &info, const VibratorIdentifier &identifier)
{
    CALL_LOG_ENTER;
    if (!SetUsage(identifier, info.usage, info.systemUsage)) {
        MISC_HILOGE("SetUsage fail");
        return DEVICE_OPERATION_FAILED;
    }
    if (g_allowedTypes.find(info.type) == g_allowedTypes.end()) {
        MISC_HILOGE("Invalid vibrate type, type:%{public}s", info.type.c_str());
        return DEVICE_OPERATION_FAILED;
    }
    return ERR_OK;
}

int32_t StartVibrateEnhanced(const CJVibrateInfo &info, const VibratorIdentifier &identifier)
{
    CALL_LOG_ENTER;
    if (CheckParameters(info, identifier)!= ERR_OK) {
        MISC_HILOGE("SetUsage fail");
        return DEVICE_OPERATION_FAILED;
    }
    if (info.type == "file") {
        return PlayVibratorCustomEnhanced(identifier, info.fd, info.offset, info.length);
    } else if (info.type == "pattern") {
        return PlayPatternEnhanced(identifier, info.vibratorPattern);
    }
    return StartVibratorOnceEnhanced(identifier, info.duration);
}

extern "C" {
    void FfiVibratorStartVibrationTime(RetVibrateTime effect, RetVibrateAttribute attribute, int32_t &code)
    {
        CJVibrateInfo info;
        std::string strType(effect.timeType);
        info.type = strType;
        info.duration = effect.duration;
        std::string strUsage(attribute.usage);
        info.usage = strUsage;
        info.systemUsage = false;
        code = StartVibrate(info);
    };

    void FfiVibratorStartVibrationPreset(RetVibratePreset effect, RetVibrateAttribute attribute, int32_t &code)
    {
        CJVibrateInfo info;
        std::string strType(effect.presetType);
        info.type = strType;
        std::string strEffectId(effect.effectId);
        info.effectId = strEffectId;
        info.count = effect.count;
        info.intensity = effect.intensity;
        std::string strUsage(attribute.usage);
        info.usage = strUsage;
        info.systemUsage = false;
        code = StartVibrate(info);
    };

    void FfiVibratorStartVibrationFile(RetVibrateFromFile effect, RetVibrateAttribute attribute, int32_t &code)
    {
        CJVibrateInfo info;
        std::string strType(effect.fileType);
        info.type = strType;
        info.fd = effect.hapticFd.fd;
        info.offset = effect.hapticFd.offSet;
        info.length = effect.hapticFd.length;
        std::string strUsage(attribute.usage);
        info.usage = strUsage;
        info.systemUsage = false;
        code = StartVibrate(info);
    };

    void FfiVibratorStopVibration(int32_t &code)
    {
        code = Cancel();
    }

    void FfiVibratorStopVibrationMode(char* vibMode, int32_t &code)
    {
        if (vibMode == nullptr) {
            code = PARAMETER_ERROR;
            return;
        }
        if (strcmp(vibMode, "time") != 0 && strcmp(vibMode, "preset") != 0) {
            code = PARAMETER_ERROR;
            return;
        }
        std::string mode(vibMode);
        code = StopVibrator(mode.c_str());
    }

    bool FfiVibratorSupportEffect(char* id, int32_t &code)
    {
        bool isSupportEffect = false;
        if (id == nullptr) {
            code = PARAMETER_ERROR;
            return isSupportEffect;
        }
        if (strcmp(id, "haptic.clock.timer") != 0) {
            code = PARAMETER_ERROR;
            return isSupportEffect;
        }
        std::string effectId(id);
        code = IsSupportEffect(effectId.c_str(), &isSupportEffect);
        return isSupportEffect;
    }

    bool FfiVibratorIsHdHapticSupported()
    {
        return IsHdHapticSupported();
    }

    FFI_EXPORT int32_t FfiVibratorStartVibrationTimeEnhanced(RetVibrateTime effect,
        RetVibrateAttributeEnhanced attribute)
    {
        CJVibrateInfo info;
        if (effect.timeType == nullptr || attribute.usage == nullptr) {
            return PARAMETER_ERROR;
        }
        std::string strType(effect.timeType);
        info.type = strType;
        info.duration = effect.duration;
        std::string strUsage(attribute.usage);
        info.usage = strUsage;
        info.systemUsage = false;

        VibratorIdentifier identifier;
        identifier.deviceId = attribute.deviceId;
        identifier.vibratorId = attribute.id;

        if (identifier.vibratorId == 0) {
            identifier.vibratorId = -1;
        }

        return StartVibrateEnhanced(info, identifier);
    }

    FFI_EXPORT int32_t FfiVibratorStartVibrationFileEnhanced(RetVibrateFromFile effect,
        RetVibrateAttributeEnhanced attribute)
    {
        if (effect.fileType == nullptr || attribute.usage == nullptr) {
            return PARAMETER_ERROR;
        }
        CJVibrateInfo info;
        std::string strType(effect.fileType);
        info.type = strType;
        info.fd = effect.hapticFd.fd;
        info.offset = effect.hapticFd.offSet;
        info.length = effect.hapticFd.length;
        std::string strUsage(attribute.usage);
        info.usage = strUsage;
        info.systemUsage = false;
        VibratorIdentifier identifier;
        identifier.deviceId = attribute.deviceId;
        identifier.vibratorId = attribute.id;

        if (identifier.vibratorId == 0) {
            identifier.vibratorId = -1;
        }

        return StartVibrateEnhanced(info, identifier);
    };

    FFI_EXPORT int32_t FfiVibratorStartVibrationPresetEnhanced(RetVibratePreset effect,
        RetVibrateAttributeEnhanced attribute)
    {
        if (effect.presetType == nullptr || effect.effectId == nullptr || attribute.usage == nullptr) {
            return PARAMETER_ERROR;
        }
        CJVibrateInfo info;
        std::string strType(effect.presetType);
        info.type = strType;
        std::string strEffectId(effect.effectId);
        info.effectId = strEffectId;
        info.count = effect.count;
        info.intensity = effect.intensity;
        std::string strUsage(attribute.usage);
        info.usage = strUsage;
        info.systemUsage = false;
        VibratorIdentifier identifier;
        identifier.deviceId = attribute.deviceId;
        identifier.vibratorId = attribute.id;

        if (identifier.vibratorId == 0) {
            identifier.vibratorId = -1;
        }
        
        if (!SetLoopCountEnhanced(identifier, info.count) || CheckParameters(info, identifier) != ERR_OK
            || info.effectId.empty()) {
            return DEVICE_OPERATION_FAILED;
        }
        return PlayPrimitiveEffect(info.effectId.c_str(), info.intensity);
    };

    FFI_EXPORT CArrRetVibratorInfo FfiVibratorGetVibratorInfo(RetVibratorInfoParam param, int32_t *code)
    {
        CArrRetVibratorInfo arrInfo = {.head = nullptr, .size = 0};
        VibratorIdentifier identifier;
        identifier.deviceId = param.deviceId;
        identifier.vibratorId = param.vibratorId;
        std::vector<VibratorInfos> vibratorInfo;
        int32_t ret = GetVibratorList(identifier, vibratorInfo);
        if (ret != SUCCESS) {
            *code = ret;
            return arrInfo;
        }
        arrInfo.size = static_cast<int64_t>(vibratorInfo.size());
        if (arrInfo.size == 0) {
            return arrInfo;
        }
        RetVibratorInfos* retValue = static_cast<RetVibratorInfos*>(
            malloc(sizeof(RetVibratorInfos) * arrInfo.size));
        if (retValue == nullptr) {
            *code = DEVICE_OPERATION_FAILED;
            return arrInfo;
        }
        int32_t flag = 0;
        bool fail = false;
        for (std::size_t i = 0; i < arrInfo.size; ++i) {
            retValue[i].deviceId = vibratorInfo[i].deviceId;
            retValue[i].vibratorId = vibratorInfo[i].vibratorId;
            if (vibratorInfo[i].deviceName.empty()) {
                retValue[i].deviceName = nullptr;
            } else {
                retValue[i].deviceName = MallocCString(vibratorInfo[i].deviceName);
                if (retValue[i].deviceName == nullptr) {
                    fail = true;
                    *code = DEVICE_OPERATION_FAILED;
                    break;
                }
            }
            retValue[i].isSupportHdHaptic = vibratorInfo[i].isSupportHdHaptic;
            retValue[i].isLocalVibrator = vibratorInfo[i].isLocalVibrator;
            flag += 1;
        }
        if (fail == true) {
            for (int32_t i = 0; i < flag; ++i) {
                free(retValue[i].deviceName);
            }
            free(retValue);
            retValue = nullptr;
        }
        arrInfo.head = retValue;
        return arrInfo;
    }

    FFI_EXPORT int32_t FfiVibratorStopVibrationEnhanced(RetVibratorInfoParam param)
    {
        VibratorIdentifier identifier;
        identifier.deviceId = param.deviceId;
        identifier.vibratorId = param.vibratorId;
        return CancelEnhanced(identifier);
    }
}
}
}