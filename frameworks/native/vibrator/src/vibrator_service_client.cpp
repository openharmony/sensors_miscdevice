/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "vibrator_service_client.h"

#include <algorithm>
#include <climits>
#include <set>
#include <thread>
#include <vector>


#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
#ifdef HIVIEWDFX_HITRACE_ENABLE
#include "hitrace_meter.h"
#endif // HIVIEWDFX_HITRACE_ENABLE
#include "iservice_registry.h"
#include "securec.h"
#include "system_ability_definition.h"

#include "death_recipient_template.h"
#include "sensors_errors.h"
#include "vibrator_decoder_creator.h"

#undef LOG_TAG
#define LOG_TAG "VibratorServiceClient"

namespace OHOS {
namespace Sensors {
static constexpr int32_t MIN_VIBRATOR_EVENT_TIME = 100;
static constexpr int32_t FREQUENCY_UPPER_BOUND = 100;
static constexpr int32_t FREQUENCY_LOWER_BOUND = -100;
static constexpr int32_t INTENSITY_UPPER_BOUND = 100;
static constexpr int32_t INTENSITY_LOWER_BOUND = 0;
static constexpr int32_t TAKE_AVERAGE = 2;
static constexpr int32_t MAX_PATTERN_EVENT_NUM = 1000;
static constexpr int32_t MAX_PATTERN_NUM = 1000;
static constexpr int32_t CURVE_POINT_NUM_MIN = 4;
static constexpr int32_t CURVE_POINT_NUM_MAX = 16;

using namespace OHOS::HiviewDFX;

namespace {
#if (defined(__aarch64__) || defined(__x86_64__))
    static const std::string DECODER_LIBRARY_PATH = "/system/lib64/platformsdk/libvibrator_decoder.z.so";
#else
    static const std::string DECODER_LIBRARY_PATH = "/system/lib/platformsdk/libvibrator_decoder.z.so";
#endif
} // namespace

VibratorServiceClient::~VibratorServiceClient()
{
    if (miscdeviceProxy_ != nullptr && serviceDeathObserver_ != nullptr) {
        auto remoteObject = miscdeviceProxy_->AsObject();
        if (remoteObject != nullptr) {
            remoteObject->RemoveDeathRecipient(serviceDeathObserver_);
        }
    }
    std::lock_guard<std::mutex> decodeLock(decodeMutex_);
    if (decodeHandle_.destroy != nullptr && decodeHandle_.handle != nullptr) {
        decodeHandle_.destroy(decodeHandle_.decoder);
        decodeHandle_.decoder = nullptr;
        decodeHandle_.Free();
    }
}

int32_t VibratorServiceClient::InitServiceClient()
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    if (miscdeviceProxy_ != nullptr) {
        MISC_HILOGD("miscdeviceProxy_ already init");
        return ERR_OK;
    }
    if (vibratorClient_ == nullptr) {
        vibratorClient_ = new (std::nothrow) VibratorClientStub();
    }
    auto sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sm == nullptr) {
        MISC_HILOGE("sm cannot be null");
        return MISC_NATIVE_SAM_ERR;
    }
    miscdeviceProxy_ = iface_cast<IMiscdeviceService>(sm->GetSystemAbility(MISCDEVICE_SERVICE_ABILITY_ID));
    if (miscdeviceProxy_ != nullptr) {
        serviceDeathObserver_ =
            new (std::nothrow) DeathRecipientTemplate(*const_cast<VibratorServiceClient *>(this));
        CHKPR(serviceDeathObserver_, MISC_NATIVE_GET_SERVICE_ERR);
        auto remoteObject = miscdeviceProxy_->AsObject();
        CHKPR(remoteObject, MISC_NATIVE_GET_SERVICE_ERR);
        remoteObject->AddDeathRecipient(serviceDeathObserver_);
        int32_t ret = TransferClientRemoteObject();
        if (ret != ERR_OK) {
            MISC_HILOGE("TransferClientRemoteObject failed, ret:%{public}d", ret);
            return ERROR;
        }
        return ERR_OK;
    }
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
    HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_EXCEPTION",
        HiSysEvent::EventType::FAULT, "PKG_NAME", "InitServiceClient", "ERROR_CODE", MISC_NATIVE_GET_SERVICE_ERR);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
    MISC_HILOGE("Get service failed");
    return MISC_NATIVE_GET_SERVICE_ERR;
}

int32_t VibratorServiceClient::TransferClientRemoteObject()
{
    auto remoteObject = vibratorClient_->AsObject();
    CHKPR(remoteObject, MISC_NATIVE_GET_SERVICE_ERR);
    CHKPR(miscdeviceProxy_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "TransferClientRemoteObject");
#endif // HIVIEWDFX_HITRACE_ENABLE
    int32_t ret = miscdeviceProxy_->TransferClientRemoteObject(remoteObject);
    WriteOtherHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_TRANSFER_CLIENT_REMOTE_OBJECT, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    return ret;
}

int32_t VibratorServiceClient::Vibrate(const VibratorIdentifier &identifier, int32_t timeOut, int32_t usage,
    bool systemUsage)
{
    MISC_HILOGD("Vibrate begin, time:%{public}d, usage:%{public}d, deviceId:%{public}d, vibratorId:%{public}d",
        timeOut, usage, identifier.deviceId, identifier.vibratorId);
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(miscdeviceProxy_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "VibrateTime");
#endif // HIVIEWDFX_HITRACE_ENABLE
    VibratorIdentifierIPC vibrateIdentifier;
    vibrateIdentifier.deviceId = identifier.deviceId;
    vibrateIdentifier.vibratorId = identifier.vibratorId;
    ret = miscdeviceProxy_->Vibrate(vibrateIdentifier, timeOut, usage, systemUsage);
    WriteVibratorHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_VIBRATE, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGE("Vibrate time failed, ret:%{public}d, time:%{public}d, usage:%{public}d", ret, timeOut, usage);
    }
    return ret;
}

int32_t VibratorServiceClient::Vibrate(const VibratorIdentifier &identifier, const std::string &effect,
    int32_t loopCount, int32_t usage, bool systemUsage)
{
    MISC_HILOGD("Vibrate begin, effect:%{public}s, loopCount:%{public}d, usage:%{public}d",
        effect.c_str(), loopCount, usage);
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(miscdeviceProxy_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "VibrateEffect");
#endif // HIVIEWDFX_HITRACE_ENABLE
    VibratorIdentifierIPC vibrateIdentifier;
    vibrateIdentifier.deviceId = identifier.deviceId;
    vibrateIdentifier.vibratorId = identifier.vibratorId;
    ret = miscdeviceProxy_->PlayVibratorEffect(vibrateIdentifier, effect, loopCount, usage, systemUsage);
    WriteVibratorHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_PLAY_VIBRATOR_EFFECT, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGE("Vibrate effect failed, ret:%{public}d, effect:%{public}s, loopCount:%{public}d, usage:%{public}d",
            ret, effect.c_str(), loopCount, usage);
    }
    return ret;
}

#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
int32_t VibratorServiceClient::PlayVibratorCustom(const VibratorIdentifier &identifier, const RawFileDescriptor &rawFd,
    int32_t usage, bool systemUsage, const VibratorParameter &parameter)
{
    MISC_HILOGD("Vibrate begin, fd:%{public}d, offset:%{public}lld, length:%{public}lld, usage:%{public}d",
        rawFd.fd, static_cast<long long>(rawFd.offset), static_cast<long long>(rawFd.length), usage);
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(miscdeviceProxy_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "PlayVibratorCustom");
#endif // HIVIEWDFX_HITRACE_ENABLE
    CustomHapticInfoIPC customHapticInfoIPC;
    customHapticInfoIPC.usage = usage;
    customHapticInfoIPC.systemUsage = systemUsage;
    customHapticInfoIPC.parameter.intensity = parameter.intensity;
    customHapticInfoIPC.parameter.frequency = parameter.frequency;
    VibratorIdentifierIPC vibrateIdentifier;
    vibrateIdentifier.deviceId = identifier.deviceId;
    vibrateIdentifier.vibratorId = identifier.vibratorId;
    ret = miscdeviceProxy_->PlayVibratorCustom(vibrateIdentifier, rawFd.fd, rawFd.offset,
        rawFd.length, customHapticInfoIPC);
    WriteVibratorHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_PLAY_VIBRATOR_CUSTOM, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGE("PlayVibratorCustom failed, ret:%{public}d, usage:%{public}d", ret, usage);
    }
    return ret;
}
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM

int32_t VibratorServiceClient::StopVibrator(const VibratorIdentifier &identifier, const std::string &mode)
{
    MISC_HILOGD("StopVibrator begin, deviceId:%{public}d, vibratorId:%{public}d, mode:%{public}s", identifier.deviceId,
        identifier.vibratorId, mode.c_str());
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(miscdeviceProxy_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "StopVibratorByMode");
#endif // HIVIEWDFX_HITRACE_ENABLE
    VibratorIdentifierIPC vibrateIdentifier;
    vibrateIdentifier.deviceId = identifier.deviceId;
    vibrateIdentifier.vibratorId = identifier.vibratorId;
    ret = miscdeviceProxy_->StopVibratorByMode(vibrateIdentifier, mode);
    WriteVibratorHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_STOP_VIBRATOR_BY_MODE, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGD("StopVibrator by mode failed, ret:%{public}d, mode:%{public}s", ret, mode.c_str());
    }
    return ret;
}

int32_t VibratorServiceClient::StopVibrator(const VibratorIdentifier &identifier)
{
    MISC_HILOGD("StopVibrator begin, deviceId:%{public}d, vibratorId:%{public}d", identifier.deviceId,
        identifier.vibratorId);
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(miscdeviceProxy_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "StopVibratorAll");
#endif // HIVIEWDFX_HITRACE_ENABLE
    VibratorIdentifierIPC vibrateIdentifier;
    vibrateIdentifier.deviceId = identifier.deviceId;
    vibrateIdentifier.vibratorId = identifier.vibratorId;
    ret = miscdeviceProxy_->StopVibrator(vibrateIdentifier);
    WriteVibratorHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_STOP_VIBRATOR, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGD("StopVibrator failed, ret:%{public}d", ret);
    }
    return ret;
}

bool VibratorServiceClient::IsHdHapticSupported(const VibratorIdentifier &identifier)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    VibratorCapacity capacity_;
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(miscdeviceProxy_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "IsHdHapticSupported");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = GetVibratorCapacity(identifier, capacity_);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGE("IsHdHapticSupported failed, ret:%{public}d", ret);
    }
    return capacity_.isSupportHdHaptic;
}

int32_t VibratorServiceClient::IsSupportEffect(const VibratorIdentifier &identifier, const std::string &effect,
    bool &state)
{
    MISC_HILOGD("IsSupportEffect begin, effect:%{public}s", effect.c_str());
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(miscdeviceProxy_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "VibrateEffect");
#endif // HIVIEWDFX_HITRACE_ENABLE
    VibratorIdentifierIPC vibrateIdentifier;
    vibrateIdentifier.deviceId = identifier.deviceId;
    vibrateIdentifier.vibratorId = identifier.vibratorId;
    ret = miscdeviceProxy_->IsSupportEffect(vibrateIdentifier, effect, state);
    WriteOtherHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_IS_SUPPORT_EFFECT, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGE("Query effect support failed, ret:%{public}d, effect:%{public}s", ret, effect.c_str());
    }
    return ret;
}

void VibratorServiceClient::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    CALL_LOG_ENTER;
    (void)object;
    {
        std::lock_guard<std::mutex> clientLock(clientMutex_);
        miscdeviceProxy_ = nullptr;
    }
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return;
    }
}

int32_t VibratorServiceClient::LoadDecoderLibrary(const std::string& path)
{
    std::lock_guard<std::mutex> decodeLock(decodeMutex_);
    if (decodeHandle_.handle != nullptr) {
        MISC_HILOGD("The library has already been loaded");
        return ERR_OK;
    }
    char libRealPath[PATH_MAX] = {};
    if (realpath(path.c_str(), libRealPath) == nullptr) {
        MISC_HILOGE("Get file real path fail");
        return ERROR;
    }
    decodeHandle_.handle = dlopen(libRealPath, RTLD_LAZY);
    if (decodeHandle_.handle == nullptr) {
        MISC_HILOGE("dlopen failed, reason:%{public}s", dlerror());
        return ERROR;
    }
    decodeHandle_.create = reinterpret_cast<IVibratorDecoder *(*)(const JsonParser &)>(
        dlsym(decodeHandle_.handle, "Create"));
    if (decodeHandle_.create == nullptr) {
        MISC_HILOGE("dlsym create failed: error: %{public}s", dlerror());
        decodeHandle_.Free();
        return ERROR;
    }
    decodeHandle_.destroy = reinterpret_cast<void (*)(IVibratorDecoder *)>
        (dlsym(decodeHandle_.handle, "Destroy"));
    if (decodeHandle_.destroy == nullptr) {
        MISC_HILOGE("dlsym destroy failed: error: %{public}s", dlerror());
        decodeHandle_.Free();
        return ERROR;
    }
    return ERR_OK;
}

int32_t VibratorServiceClient::PreProcess(const VibratorFileDescription &fd, VibratorPackage &package)
{
    if (LoadDecoderLibrary(DECODER_LIBRARY_PATH) != 0) {
        MISC_HILOGE("LoadDecoderLibrary fail");
        return ERROR;
    }
    RawFileDescriptor rawFd = {
        .fd = fd.fd,
        .offset = fd.offset,
        .length = fd.length
    };
    JsonParser parser(rawFd);
    decodeHandle_.decoder = decodeHandle_.create(parser);
    CHKPR(decodeHandle_.decoder, ERROR);
    VibratePackage pkg = {};
    if (decodeHandle_.decoder->DecodeEffect(rawFd, parser, pkg) != 0) {
        MISC_HILOGE("DecodeEffect fail");
        decodeHandle_.destroy(decodeHandle_.decoder);
        decodeHandle_.decoder = nullptr;
        return ERROR;
    }
    decodeHandle_.destroy(decodeHandle_.decoder);
    decodeHandle_.decoder = nullptr;
    return ConvertVibratorPackage(pkg, package);
}

int32_t VibratorServiceClient::GetDelayTime(const VibratorIdentifier &identifier, int32_t &delayTime)
{
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(miscdeviceProxy_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "GetDelayTime");
#endif // HIVIEWDFX_HITRACE_ENABLE
    VibratorIdentifierIPC vibrateIdentifier;
    vibrateIdentifier.deviceId = identifier.deviceId;
    vibrateIdentifier.vibratorId = identifier.vibratorId;
    ret = miscdeviceProxy_->GetDelayTime(vibrateIdentifier, delayTime);
    WriteOtherHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_GET_DELAY_TIME, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGE("GetDelayTime failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t VibratorServiceClient::InitPlayPattern(const VibratorIdentifier &identifier, const VibratorPattern &pattern,
    int32_t usage, bool systemUsage, const VibratorParameter &parameter)
{
    VibratePattern vibratePattern = {};
    vibratePattern.startTime = pattern.time;
    if (pattern.eventNum < 0 || pattern.eventNum > MAX_PATTERN_EVENT_NUM) {
        MISC_HILOGE("VibratorPattern's eventNum is invalid, eventNum:%{public}d", pattern.eventNum);
        return ERROR;
    }
    for (int32_t i = 0; i < pattern.eventNum; ++i) {
        if (pattern.events == nullptr) {
            MISC_HILOGE("VibratorPattern's events is null");
            return ERROR;
        }
        VibrateEvent event;
        event.tag = static_cast<VibrateTag>(pattern.events[i].type);
        event.time = pattern.events[i].time;
        event.duration = pattern.events[i].duration;
        event.intensity = pattern.events[i].intensity;
        event.frequency = pattern.events[i].frequency;
        event.index = pattern.events[i].index;
        for (int32_t j = 0; j < pattern.events[i].pointNum; ++j) {
            if (pattern.events[i].points == nullptr) {
                MISC_HILOGE("VibratorEvent's points is null");
                continue;
            }
            VibrateCurvePoint point;
            point.time = pattern.events[i].points[j].time;
            point.intensity = pattern.events[i].points[j].intensity;
            point.frequency = pattern.events[i].points[j].frequency;
            event.points.emplace_back(point);
        }
        vibratePattern.events.emplace_back(event);
        vibratePattern.patternDuration = pattern.patternDuration;
    }
    CustomHapticInfoIPC customHapticInfoIPC;
    customHapticInfoIPC.usage = usage;
    customHapticInfoIPC.systemUsage = systemUsage;
    customHapticInfoIPC.parameter.intensity = parameter.intensity;
    customHapticInfoIPC.parameter.frequency = parameter.frequency;
    customHapticInfoIPC.parameter.sessionId = parameter.sessionId;
    VibratorIdentifierIPC vibrateIdentifier;
    vibrateIdentifier.deviceId = identifier.deviceId;
    vibrateIdentifier.vibratorId = identifier.vibratorId;
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(miscdeviceProxy_, ERROR);
    int32_t ret = miscdeviceProxy_->PlayPattern(vibrateIdentifier, vibratePattern, customHapticInfoIPC);
    WriteOtherHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_PLAY_PATTERN, ret);
    return ret;
}

int32_t VibratorServiceClient::PlayPattern(const VibratorIdentifier &identifier, const VibratorPattern &pattern,
    int32_t usage, bool systemUsage, const VibratorParameter &parameter)
{
    MISC_HILOGD("Vibrate begin, usage:%{public}d", usage);
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "PlayPattern");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = InitPlayPattern(identifier, pattern, usage, systemUsage, parameter);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGE("PlayPattern failed, ret:%{public}d, usage:%{public}d", ret, usage);
    }
    return ret;
}

int32_t VibratorServiceClient::ConvertVibratorPackage(const VibratorPackage& inPkg, VibratePackageIPC &outPkg)
{
    outPkg.patternNum = inPkg.patternNum;
    outPkg.packageDuration = inPkg.packageDuration;
    for (int32_t i = 0; i < inPkg.patternNum; ++i) {
        if (inPkg.patterns == nullptr) {
            MISC_HILOGE("VibratorPackage's patterns is null");
            return ERROR;
        }
        VibratePattern vibratePattern = {};
        vibratePattern.startTime = inPkg.patterns[i].time;
        vibratePattern.patternDuration = inPkg.patterns[i].patternDuration;
        for (int32_t j = 0; j < inPkg.patterns[i].eventNum; ++j) {
            if (inPkg.patterns[i].events == nullptr) {
                MISC_HILOGE("vibratePattern's events is null");
                return ERROR;
            }
            VibrateEvent event;
            event.tag = static_cast<VibrateTag>(inPkg.patterns[i].events[j].type);
            event.time = inPkg.patterns[i].events[j].time;
            event.duration = inPkg.patterns[i].events[j].duration;
            event.intensity = inPkg.patterns[i].events[j].intensity;
            event.frequency = inPkg.patterns[i].events[j].frequency;
            event.index = inPkg.patterns[i].events[j].index;
            for (int32_t k = 0; k < inPkg.patterns[i].events[j].pointNum; ++k) {
                if (inPkg.patterns[i].events[j].points == nullptr) {
                    MISC_HILOGE("VibratorEvent's points is null");
                    continue;
                }
                VibrateCurvePoint point;
                point.time = inPkg.patterns[i].events[j].points[k].time;
                point.intensity = inPkg.patterns[i].events[j].points[k].intensity;
                point.frequency = inPkg.patterns[i].events[j].points[k].frequency;
                event.points.emplace_back(point);
            }
            vibratePattern.events.emplace_back(event);
            vibratePattern.patternDuration = inPkg.patterns[i].patternDuration;
        }
        outPkg.patterns.emplace_back(vibratePattern);
    }
    return ERR_OK;
}

int32_t VibratorServiceClient::PlayPackageBySessionId(const VibratorIdentifier &identifier,
    const VibratorEffectParameter &parameter, const VibratorPackage &package)
{
    MISC_HILOGD("PlayPackageBySessionId begin, sessionId:%{public}d", parameter.sessionId);
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    VibratePackageIPC packageIPC;
    if (ConvertVibratorPackage(package, packageIPC) != ERR_OK) {
        MISC_HILOGE("VibratorPackage parameter invalid");
        return PARAMETER_ERROR;
    }
    CustomHapticInfoIPC customHapticInfoIPC;
    customHapticInfoIPC.usage = parameter.usage;
    customHapticInfoIPC.systemUsage = parameter.systemUsage;
    customHapticInfoIPC.parameter.intensity = parameter.vibratorParameter.intensity;
    customHapticInfoIPC.parameter.frequency = parameter.vibratorParameter.frequency;
    customHapticInfoIPC.parameter.sessionId = parameter.sessionId;
    VibratorIdentifierIPC vibrateIdentifier;
    vibrateIdentifier.deviceId = identifier.deviceId;
    vibrateIdentifier.vibratorId = identifier.vibratorId;
    std::lock_guard<std::mutex> clientLock(clientMutex_);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "PlayPackageBySessionId");
#endif // HIVIEWDFX_HITRACE_ENABLE
    CHKPR(miscdeviceProxy_, ERROR);
    ret = miscdeviceProxy_->PlayPackageBySessionId(vibrateIdentifier, packageIPC, customHapticInfoIPC);
    WriteVibratorHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_PLAY_PACKAGE_BY_SESSION_ID, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGD("PlayPackageBySessionId failed, ret:%{public}d, sessionId:%{public}d", ret, parameter.sessionId);
    }
    return ret;
}

int32_t VibratorServiceClient::StopVibrateBySessionId(const VibratorIdentifier &identifier, uint32_t sessionId)
{
    MISC_HILOGD("StopVibrateBySessionId begin, sessionId:%{public}d", sessionId);
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "StopVibrateBySessionId");
#endif // HIVIEWDFX_HITRACE_ENABLE
    VibratorIdentifierIPC vibrateIdentifier;
    vibrateIdentifier.deviceId = identifier.deviceId;
    vibrateIdentifier.vibratorId = identifier.vibratorId;
    CHKPR(miscdeviceProxy_, ERROR);
    ret = miscdeviceProxy_->StopVibrateBySessionId(vibrateIdentifier, sessionId);
    WriteVibratorHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_STOP_VIBRATE_BY_SESSION_ID, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGD("StopVibrateBySessionId failed, ret:%{public}d, sessionId:%{public}d", ret, sessionId);
    }
    return ret;
}

int32_t VibratorServiceClient::ConvertVibratorPackage(const VibratePackage& inPkg,
    VibratorPackage &outPkg)
{
    inPkg.Dump();
    int32_t patternSize = static_cast<int32_t>(inPkg.patterns.size());
    VibratorPattern *patterns = (VibratorPattern *)malloc(sizeof(VibratorPattern) * patternSize);
    CHKPR(patterns, ERROR);
    outPkg.patternNum = patternSize;
    int32_t clientPatternDuration = 0;
    for (int32_t i = 0; i < patternSize; ++i) {
        patterns[i].time = inPkg.patterns[i].startTime;
        auto vibrateEvents = inPkg.patterns[i].events;
        int32_t eventSize = static_cast<int32_t>(vibrateEvents.size());
        patterns[i].eventNum = eventSize;
        VibratorEvent *events = (VibratorEvent *)malloc(sizeof(VibratorEvent) * eventSize);
        if (events == nullptr) {
            free(patterns);
            patterns = nullptr;
            return ERROR;
        }
        for (int32_t j = 0; j < eventSize; ++j) {
            events[j].type = static_cast<VibratorEventType >(vibrateEvents[j].tag);
            events[j].time = vibrateEvents[j].time;
            events[j].duration = vibrateEvents[j].duration;
            events[j].intensity = vibrateEvents[j].intensity;
            events[j].frequency = vibrateEvents[j].frequency;
            events[j].index = vibrateEvents[j].index;
            auto vibratePoints = vibrateEvents[j].points;
            events[j].pointNum = static_cast<int32_t>(vibratePoints.size());
            VibratorCurvePoint *points = (VibratorCurvePoint *)malloc(sizeof(VibratorCurvePoint) * events[j].pointNum);
            if (points == nullptr) {
                free(patterns);
                patterns = nullptr;
                free(events);
                events = nullptr;
                return ERROR;
            }
            for (int32_t k = 0; k < events[j].pointNum; ++k) {
                points[k].time = vibratePoints[k].time;
                points[k].intensity  = vibratePoints[k].intensity;
                points[k].frequency  = vibratePoints[k].frequency;
            }
            events[j].points = points;
            clientPatternDuration = events[j].time + events[j].duration;
        }
        patterns[i].events = events;
        patterns[i].patternDuration = clientPatternDuration;
    }
    outPkg.patterns = patterns;
    outPkg.packageDuration = inPkg.packageDuration;
    return ERR_OK;
}

int32_t VibratorServiceClient::FreeVibratorPackage(VibratorPackage &package)
{
    int32_t patternSize = package.patternNum;
    if ((patternSize <= 0) || (package.patterns == nullptr)) {
        MISC_HILOGW("Patterns is not need to free, pattern size:%{public}d", patternSize);
        return ERROR;
    }
    auto patterns = package.patterns;
    for (int32_t i = 0; i < patternSize; ++i) {
        int32_t eventNum = patterns[i].eventNum;
        if ((eventNum <= 0) || (patterns[i].events == nullptr)) {
            MISC_HILOGW("Events is not need to free, event size:%{public}d", eventNum);
            continue;
        }
        auto events = patterns[i].events;
        for (int32_t j = 0; j < eventNum; ++j) {
            if (events[j].points != nullptr) {
                free(events[j].points);
                events[j].points = nullptr;
            }
        }
        free(events);
        events = nullptr;
    }
    free(patterns);
    patterns = nullptr;
    return ERR_OK;
}

int32_t VibratorServiceClient::PlayPrimitiveEffect(const VibratorIdentifier &identifier, const std::string &effect,
    const PrimitiveEffect &primitiveEffect)
{
    MISC_HILOGD("Vibrate begin, effect:%{public}s, intensity:%{public}d, usage:%{public}d, count:%{public}d",
        effect.c_str(), primitiveEffect.intensity, primitiveEffect.usage, primitiveEffect.count);
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(miscdeviceProxy_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "PlayPrimitiveEffect");
#endif // HIVIEWDFX_HITRACE_ENABLE
    PrimitiveEffectIPC primitiveEffectIPC;
    primitiveEffectIPC.intensity = primitiveEffect.intensity;
    primitiveEffectIPC.usage = primitiveEffect.usage;
    primitiveEffectIPC.systemUsage = primitiveEffect.systemUsage;
    primitiveEffectIPC.count = primitiveEffect.count;
    VibratorIdentifierIPC vibrateIdentifier;
    vibrateIdentifier.deviceId = identifier.deviceId;
    vibrateIdentifier.vibratorId = identifier.vibratorId;
    ret = miscdeviceProxy_->PlayPrimitiveEffect(vibrateIdentifier, effect, primitiveEffectIPC);
    WriteOtherHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_PLAY_PRIMITIVE_EFFECT, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGE("Play primitive effect failed, ret:%{public}d, effect:%{public}s, intensity:%{public}d,"
            "usage:%{public}d, count:%{public}d", ret, effect.c_str(), primitiveEffect.intensity,
            primitiveEffect.usage, primitiveEffect.count);
    }
    return ret;
}

int32_t VibratorServiceClient::GetVibratorCapacity(const VibratorIdentifier &identifier, VibratorCapacity &capacity)
{
    CHKPR(miscdeviceProxy_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "GetVibratorCapacity");
#endif // HIVIEWDFX_HITRACE_ENABLE
    VibratorIdentifierIPC vibrateIdentifier;
    vibrateIdentifier.deviceId = identifier.deviceId;
    vibrateIdentifier.vibratorId = identifier.vibratorId;
    int32_t ret = miscdeviceProxy_->GetVibratorCapacity(vibrateIdentifier, capacity);
    WriteVibratorHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_GET_VIBRATOR_CAPACITY, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    capacity.Dump();
    return ret;
}

bool VibratorServiceClient::IsSupportVibratorCustom(const VibratorIdentifier &identifier)
{
    MISC_HILOGD("Vibrate begin");
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
    }
    VibratorCapacity capacity_;
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(miscdeviceProxy_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "IsSupportVibratorCustom");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = GetVibratorCapacity(identifier, capacity_);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGE("Is support vibrator custom, ret:%{public}d", ret);
    }
    return (capacity_.isSupportHdHaptic || capacity_.isSupportPresetMapping || capacity_.isSupportTimeDelay);
}

int32_t VibratorServiceClient::SeekTimeOnPackage(int32_t seekTime, const VibratorPackage &completePackage,
    VibratorPackage &seekPackage)
{
    VibratePackage convertPackage = {};
    ConvertSeekVibratorPackage(completePackage, convertPackage, seekTime);
    return ConvertVibratorPackage(convertPackage, seekPackage);
}

int32_t VibratorServiceClient::SubscribeVibratorPlugInfo(const VibratorUser *user)
{
    MISC_HILOGD("In, SubscribeVibratorPlugInfo");
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);

    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }

    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    auto status = subscribeSet_.insert(user);
    if (!status.second) {
        MISC_HILOGD("User has been subscribed");
    } else {
        std::lock_guard<std::mutex> clientLock(clientMutex_);
        auto remoteObject = vibratorClient_->AsObject();
        CHKPR(remoteObject, MISC_NATIVE_GET_SERVICE_ERR);
        CHKPR(miscdeviceProxy_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
        StartTrace(HITRACE_TAG_SENSORS, "SubscribeVibratorPlugInfo");
#endif // HIVIEWDFX_HITRACE_ENABLE
        ret = miscdeviceProxy_->SubscribeVibratorPlugInfo(remoteObject);
        WriteOtherHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_SUBSCRIBE_VIBRATOR_PLUG_INFO, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
        FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
        if (ret != ERR_OK) {
            MISC_HILOGE("Subscribe vibrator plug info failed");
            return ret;
        }
    }
    return OHOS::Sensors::SUCCESS;
}

int32_t VibratorServiceClient::UnsubscribeVibratorPlugInfo(const VibratorUser *user)
{
    MISC_HILOGD("In, UnsubscribeVibratorPlugInfo");
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);

    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    if (subscribeSet_.find(user) == subscribeSet_.end()) {
        MISC_HILOGD("Deactivate user first");
        return OHOS::Sensors::ERROR;
    }
    subscribeSet_.erase(user);
    return OHOS::Sensors::SUCCESS;
}

std::set<RecordVibratorPlugCallback> VibratorServiceClient::GetSubscribeUserCallback(int32_t deviceId)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    std::set<RecordVibratorPlugCallback> callback;
    for (const auto &it : subscribeSet_) {
        auto ret = callback.insert(it->callback);
        if (!ret.second) {
            MISC_HILOGD("callback insert fail");
        }
    }
    return callback;
}

bool VibratorServiceClient::HandleVibratorData(VibratorStatusEvent statusEvent) __attribute__((no_sanitize("cfi")))
{
    CALL_LOG_ENTER;
    if (statusEvent.type == PLUG_STATE_EVENT_PLUG_OUT) {
        std::lock_guard<std::mutex> VibratorEffectLock(vibratorEffectMutex_);
        for (auto it = vibratorEffectMap_.begin(); it != vibratorEffectMap_.end();) {
            if (it->first.deviceId == statusEvent.deviceId) {
                it = vibratorEffectMap_.erase(it);
            } else {
                ++it;
            }
        }
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    auto callbacks = GetSubscribeUserCallback(statusEvent.deviceId);
    MISC_HILOGD("callbacks.size() = %{public}zu", callbacks.size());
    MISC_HILOGD("VibratorStatusEvent = [type = %{public}d, deviceId = %{public}d]",
                statusEvent.type, statusEvent.deviceId);
    for (const auto &callback : callbacks) {
        MISC_HILOGD("callback is run");
        if (callback != nullptr)
            callback(&statusEvent);
    }
    return true;
}

void VibratorServiceClient::SetUsage(const VibratorIdentifier &identifier, int32_t usage, bool systemUsage)
{
    std::lock_guard<std::mutex> VibratorEffectLock(vibratorEffectMutex_);
    auto it = vibratorEffectMap_.find(identifier);
    if (it != vibratorEffectMap_.end()) {
        it->second.usage = usage;
        it->second.systemUsage = systemUsage;
    } else {
        VibratorEffectParameter param = {
            .usage = usage,
            .systemUsage = systemUsage,
        };
        vibratorEffectMap_[identifier] = param;
    }
}

void VibratorServiceClient::SetLoopCount(const VibratorIdentifier &identifier, int32_t count)
{
    std::lock_guard<std::mutex> VibratorEffectLock(vibratorEffectMutex_);
    auto it = vibratorEffectMap_.find(identifier);
    if (it != vibratorEffectMap_.end()) {
        it->second.loopCount = count;
    } else {
        VibratorEffectParameter param = {
            .loopCount = count,
        };
        vibratorEffectMap_[identifier] = param;
    }
}

void VibratorServiceClient::SetParameters(const VibratorIdentifier &identifier, const VibratorParameter &parameter)
{
    std::lock_guard<std::mutex> VibratorEffectLock(vibratorEffectMutex_);
    auto it = vibratorEffectMap_.find(identifier);
    if (it != vibratorEffectMap_.end()) {
        it->second.vibratorParameter = parameter;
    } else {
        VibratorEffectParameter param = {
            .vibratorParameter = parameter,
        };
        vibratorEffectMap_[identifier] = param;
    }
}

VibratorEffectParameter VibratorServiceClient::GetVibratorEffectParameter(const VibratorIdentifier &identifier)
{
    std::lock_guard<std::mutex> VibratorEffectLock(vibratorEffectMutex_);
    auto it = vibratorEffectMap_.find(identifier);
    if (it != vibratorEffectMap_.end()) {
        return it->second;
    }
    return VibratorEffectParameter();
}

void VibratorServiceClient::ConvertSeekVibratorPackage(const VibratorPackage &completePackage,
    VibratePackage &convertPackage, int32_t seekTime)
{
    convertPackage.packageDuration = completePackage.packageDuration;
    if (completePackage.patternNum < 0 || completePackage.patternNum > MAX_PATTERN_NUM) {
        MISC_HILOGE("completePackage.patternNum is invalid, patternNum:%{public}d", completePackage.patternNum);
        return;
    }
    for (int32_t i = 0; i < completePackage.patternNum; ++i) {
        VibratePattern vibratePattern = {};
        int32_t patternStartTime = completePackage.patterns[i].time;
        if (patternStartTime >= seekTime) {
            ConvertVibratorPattern(completePackage.patterns[i], vibratePattern);
            convertPackage.patterns.emplace_back(vibratePattern);
            continue;
        }
        vibratePattern.startTime = seekTime;
        for (int32_t j = 0; j < completePackage.patterns[i].eventNum; ++j) {
            VibrateEvent vibrateEvent = {};
            if (SkipEventAndConvertVibratorEvent(completePackage.patterns[i].events[j], vibratePattern,
                                                 patternStartTime, vibrateEvent)) {
                convertPackage.packageDuration -= completePackage.patterns[i].events[j].duration;
                convertPackage.packageDuration += vibrateEvent.duration;
                convertPackage.packageDuration = convertPackage.packageDuration < 0 ? 0
                                                                                    : convertPackage.packageDuration;
                continue;
            }
            vibrateEvent.tag = static_cast<VibrateTag>(completePackage.patterns[i].events[j].type);
            vibrateEvent.time = completePackage.patterns[i].events[j].time + patternStartTime - seekTime;
            vibrateEvent.duration = completePackage.patterns[i].events[j].duration;
            vibrateEvent.intensity = completePackage.patterns[i].events[j].intensity;
            vibrateEvent.frequency = completePackage.patterns[i].events[j].frequency;
            vibrateEvent.index = completePackage.patterns[i].events[j].index;
            for (size_t k = 0; k < static_cast<uint32_t>(completePackage.patterns[i].events[j].pointNum); ++k) {
                VibrateCurvePoint vibrateCurvePoint = {};
                vibrateCurvePoint.time = completePackage.patterns[i].events[j].points[k].time;
                vibrateCurvePoint.intensity = completePackage.patterns[i].events[j].points[k].intensity;
                vibrateCurvePoint.frequency = completePackage.patterns[i].events[j].points[k].frequency;
                vibrateEvent.points.emplace_back(vibrateCurvePoint);
            }
            vibratePattern.patternDuration += vibrateEvent.duration;
            vibratePattern.events.emplace_back(vibrateEvent);
        }
        if (vibratePattern.events.empty()) {
            continue;
        }
        convertPackage.patterns.emplace_back(vibratePattern);
    }
}

void VibratorServiceClient::ConvertVibratorPattern(const VibratorPattern &vibratorPattern,
    VibratePattern &vibratePattern)
{
    vibratePattern.startTime = vibratorPattern.time;
    vibratePattern.patternDuration = vibratorPattern.patternDuration;
    if (vibratorPattern.eventNum < 0 || vibratorPattern.eventNum > MAX_PATTERN_EVENT_NUM) {
        MISC_HILOGE("VibratorPattern's eventNum is invalid, eventNum:%{public}d", vibratorPattern.eventNum);
        return;
    }
    for (int32_t j = 0; j < vibratorPattern.eventNum; ++j) {
        VibrateEvent vibrateEvent = {};
        vibrateEvent.tag = static_cast<VibrateTag>(vibratorPattern.events[j].type);
        vibrateEvent.time = vibratorPattern.events[j].time;
        vibrateEvent.duration = vibratorPattern.events[j].duration;
        vibrateEvent.intensity = vibratorPattern.events[j].intensity;
        vibrateEvent.frequency = vibratorPattern.events[j].frequency;
        vibrateEvent.index = vibratorPattern.events[j].index;
        for (size_t k = 0; k < static_cast<uint32_t>(vibratorPattern.events[j].pointNum); ++k) {
            VibrateCurvePoint vibrateCurvePoint = {};
            vibrateCurvePoint.time = vibratorPattern.events[j].points[k].time;
            vibrateCurvePoint.intensity = vibratorPattern.events[j].points[k].intensity;
            vibrateCurvePoint.frequency = vibratorPattern.events[j].points[k].frequency;
            vibrateEvent.points.emplace_back(vibrateCurvePoint);
        }
        vibratePattern.events.emplace_back(vibrateEvent);
    }
}

int32_t VibratorServiceClient::ModulatePackage(const VibratorEvent &modulationCurve,
    const VibratorPackage &beforeModulationPackage, VibratorPackage &afterModulationPackage)
{
    if (beforeModulationPackage.patternNum <= 0 || beforeModulationPackage.patterns == nullptr) {
        MISC_HILOGE("ModulatePackage failed: patternNum is less than 0 or patterns is null");
        return ERROR;
    }
    if (modulationCurve.time < 0 || modulationCurve.duration < 0 || modulationCurve.pointNum <= 0
        || modulationCurve.points == nullptr) {
        MISC_HILOGE("ModulatePackage failed: invalid modulationCurve, time: %{public}d, duration: %{public}d, "
            "pointNum: %{public}d", modulationCurve.time, modulationCurve.duration, modulationCurve.pointNum);
        return ERROR;
    }
    afterModulationPackage = beforeModulationPackage;
    afterModulationPackage.patterns = nullptr;
    VibratorPattern *vibratePattern = nullptr;
    vibratePattern = static_cast<VibratorPattern *>(
        calloc(beforeModulationPackage.patternNum, sizeof(VibratorPattern)));
    if (vibratePattern == nullptr) {
        MISC_HILOGE("ModulatePackage failed: failure of memory allocation for VibratePattern");
        return ERROR;
    }
    for (int32_t i = 0; i < beforeModulationPackage.patternNum; i++) {
        const VibratorPattern &beforeModPattern = beforeModulationPackage.patterns[i];
        VibratorPattern &afterModPattern = vibratePattern[i];
        afterModPattern.events = nullptr;
        if (ModulateVibratorPattern(modulationCurve, beforeModPattern, afterModPattern) != ERR_OK) {
            MISC_HILOGE("ModulatePackage failed: failure of ModulateVibratorPattern");
            FreePartiallyAllocatedVibratorPatterns(&vibratePattern, i);
            return ERROR;
        }
    }
    afterModulationPackage.patterns = vibratePattern;
    return ERR_OK;
}

/**
 * fields 'time' in VibratorPattern,VibratorEvent and VibratorCurvePoint is shown in following figure
 *        |--------------------------------|-------------------------|-------------------------|----------------
 *   VibratorPattern.time          when event happens     when curve point 1 happens    when curve point 2 happens
 *        |<-------VibratorEvent.time----->|
 *                                                                   |<--curvePoint 1's time-->|
 *                                                                    (VibratorCurvePoint.time)
 */
int32_t VibratorServiceClient::ModulateVibratorPattern(const VibratorEvent &modulationCurve,
    const VibratorPattern &beforeModulationPattern, VibratorPattern &afterModulationPattern)
{
    if (beforeModulationPattern.eventNum <= 0 || beforeModulationPattern.events == nullptr) {
        MISC_HILOGE("ModulatePackage failed due to invalid parameter");
        return ERROR;
    }
    VibratorEvent* eventsAfterMod = static_cast<VibratorEvent *>(
        calloc(beforeModulationPattern.eventNum, sizeof(VibratorEvent)));
    if (eventsAfterMod == nullptr) {
        MISC_HILOGE("ModulatePackage failed due to failure of memory allocation for VibratorEvent");
        return ERROR;
    }
    for (int i = 0; i < beforeModulationPattern.eventNum; i++) {
        eventsAfterMod[i].points = nullptr;
        if (ModulateVibratorEvent(modulationCurve, beforeModulationPattern.time,
            beforeModulationPattern.events[i], eventsAfterMod[i]) != ERR_OK) {
            MISC_HILOGE("ModulatePackage failed due to failure of handling VibrateEvent");
            FreePartiallyAllocatedVibratorEvents(&eventsAfterMod, i);
            afterModulationPattern.events = nullptr;
            return ERROR;
        }
    }
    afterModulationPattern = beforeModulationPattern;
    afterModulationPattern.events = eventsAfterMod;
    return ERR_OK;
}

void VibratorServiceClient::FreePartiallyAllocatedVibratorPatterns(VibratorPattern** patterns, const int32_t partialIdx)
{
    if (patterns == nullptr || *patterns == nullptr) {
        MISC_HILOGW("FreePartiallyAllocatedVibratorPatterns failed because patterns is null");
        return;
    }
    for (int32_t i = partialIdx; i >= 0; i--) {
        FreePartiallyAllocatedVibratorEvents(&((*patterns)[i].events), (*patterns)[i].eventNum - 1);
        (*patterns)[i].events = nullptr;
    }
    free(*patterns);
    *patterns = nullptr;
}

void VibratorServiceClient::FreePartiallyAllocatedVibratorEvents(VibratorEvent** events, const int32_t partialIdx)
{
    if (events == nullptr || *events == nullptr) {
        MISC_HILOGW("FreePartiallyAllocatedVibratorEvents failed because events is null");
        return;
    }
    for (int32_t i = partialIdx; i >= 0; i--) {
        free((*events)[i].points);
        (*events)[i].points = nullptr;
    }
    free(*events);
    *events = nullptr;
}

int32_t VibratorServiceClient::ModulateVibratorEvent(const VibratorEvent &modulationCurve,
    const int32_t patternStartTime, const VibratorEvent &beforeModulationEvent, VibratorEvent &afterModulationEvent)
{
    std::vector<VibratorCurveInterval> modInterval;
    ConvertVibratorEventsToCurveIntervals(modulationCurve, 0, modInterval);
    if (beforeModulationEvent.type != EVENT_TYPE_CONTINUOUS || beforeModulationEvent.pointNum == 0 ||
        beforeModulationEvent.points == nullptr) {
        return ModulateEventWithoutCurvePoints(modInterval, patternStartTime,
            beforeModulationEvent, afterModulationEvent);
    } else {
        return ModulateEventWithCurvePoints(modInterval, patternStartTime,
            beforeModulationEvent, afterModulationEvent);
    }
}

int32_t VibratorServiceClient::ModulateEventWithoutCurvePoints(std::vector<VibratorCurveInterval>& modInterval,
    const int32_t patternStartTime, const VibratorEvent &beforeModEvent, VibratorEvent &afterModEvent)
{
    afterModEvent = beforeModEvent;
    afterModEvent.pointNum = 0;
    afterModEvent.points = nullptr;
    int32_t startTime = beforeModEvent.time + patternStartTime;
    int32_t idx = 0;
    BinarySearchInterval(modInterval, startTime, idx);
    if (idx >= 0) {
        afterModEvent.intensity = RestrictIntensityRange(
            beforeModEvent.intensity * modInterval[idx].intensity / (INTENSITY_UPPER_BOUND - INTENSITY_LOWER_BOUND));
        afterModEvent.frequency = RestrictFrequencyRange(beforeModEvent.frequency + modInterval[idx].frequency);
    }
    return ERR_OK;
}

// absoluteTime = VibratorPattern::time + VibratorEvent::time + VibratorCurvePoint::time
int32_t VibratorServiceClient::ModulateEventWithCurvePoints(std::vector<VibratorCurveInterval>& modInterval,
    const int patternStartTime, const VibratorEvent &beforeModulationEvent, VibratorEvent &afterModulationEvent)
{
    if (beforeModulationEvent.pointNum == 0 || beforeModulationEvent.points == nullptr) {
        MISC_HILOGE("ModulateContinuousVibratorEvent: invalid event, event should hava curve points");
        return ERROR;
    }
    if (beforeModulationEvent.pointNum < CURVE_POINT_NUM_MIN || beforeModulationEvent.pointNum > CURVE_POINT_NUM_MAX) {
        MISC_HILOGE("ModulateContinuousVibratorEvent: invalid event, count of curve points should range from "
            "%{public}d to %{public}d", CURVE_POINT_NUM_MIN, CURVE_POINT_NUM_MAX);
        return ERROR;
    }
    VibratorCurvePoint* curvePoints = GetCurveListAfterModulation(modInterval, beforeModulationEvent, patternStartTime);
    if (curvePoints == nullptr) {
        MISC_HILOGE("ModulateContinuousVibratorEvent failed due to failure of GetCurveListAfterModulation");
        return ERROR;
    }
    afterModulationEvent = beforeModulationEvent;
    afterModulationEvent.points = curvePoints;
    return ERR_OK;
}

VibratorCurvePoint* VibratorServiceClient::GetCurveListAfterModulation(
    const std::vector<VibratorCurveInterval>& modInterval, const VibratorEvent &beforeModEvent,
    const int32_t patternOffset)
{
    // There are atmost (modulationCurve.pointNum + beforeModulationEvent.pointNum + 1)
    // VibratorCurvePoints after modulation
    VibratorCurvePoint* curvePoints = static_cast<VibratorCurvePoint *>(
        calloc(beforeModEvent.pointNum, sizeof(VibratorCurvePoint)));
    if (curvePoints == nullptr) {
        MISC_HILOGE("ModulateVibratorEvent failed due to failure of memory allocation for VibratorCurvePoint");
        return nullptr;
    }
    int32_t modIdx = 0;
    const int32_t startTimeOffset = patternOffset + beforeModEvent.time;
    const int32_t intensityRangeLen = INTENSITY_UPPER_BOUND - INTENSITY_LOWER_BOUND;
    for (int32_t curveIdx = 0; curveIdx < beforeModEvent.pointNum; curveIdx++) {
        const VibratorCurvePoint& beforeModCurvePoint = beforeModEvent.points[curveIdx];
        curvePoints[curveIdx] = beforeModCurvePoint;
        BinarySearchInterval(modInterval, startTimeOffset + beforeModCurvePoint.time, modIdx);
        if (modIdx >= 0) {
            curvePoints[curveIdx].intensity = RestrictIntensityRange(
                beforeModCurvePoint.intensity * modInterval[modIdx].intensity / intensityRangeLen);
            curvePoints[curveIdx].frequency = RestrictFrequencyRange(
                beforeModCurvePoint.frequency + modInterval[modIdx].frequency);
        }
    }
    return curvePoints;
}

void VibratorServiceClient::BinarySearchInterval(
    const std::vector<VibratorCurveInterval>& interval, const int32_t val, int32_t& idx)
{
    if (val < interval.begin()->beginTime || val > interval.rbegin()->endTime) {
        idx = -1;
        return;
    }
    if (val >= interval.begin()->beginTime && val <= interval.begin()->endTime) {
        idx = 0;
        return;
    }
    int32_t headIdx = 0;
    int32_t tailIdx = static_cast<int32_t>(interval.size() - 1);
    while (tailIdx - headIdx > 1) {
        int32_t middleIdx = ((tailIdx - headIdx) / TAKE_AVERAGE) + headIdx;
        if (interval[middleIdx].endTime < val) {
            headIdx = middleIdx;
        } else {
            tailIdx = middleIdx;
        }
    }
    idx = tailIdx;
    return;
}

void VibratorServiceClient::ConvertVibratorEventsToCurveIntervals(
    const VibratorEvent &vibratorEvent, const int patternTimeOffset, std::vector<VibratorCurveInterval>& curveInterval)
{
    int32_t fullOffset = patternTimeOffset + vibratorEvent.time;
    const VibratorCurvePoint* curvePoints = vibratorEvent.points;
    for (int32_t i = 0; i < vibratorEvent.pointNum; i++) {
        if (curvePoints[i].time < vibratorEvent.duration) {
            int32_t beginTime = fullOffset + curvePoints[i].time;
            int32_t endTime = fullOffset + ((i + 1) < vibratorEvent.pointNum ?
                std::min(curvePoints[i + 1].time, vibratorEvent.duration) : vibratorEvent.duration);
            int32_t frequency = curvePoints[i].frequency;
            int32_t intensity = curvePoints[i].intensity;
            curveInterval.emplace_back(VibratorCurveInterval{
                .beginTime = beginTime, .endTime = endTime, .intensity = intensity, .frequency = frequency});
        } else {
            break;
        }
    }
}

void VibratorServiceClient::ModulateSingleCurvePoint(const VibratorCurveInterval &modulationInterval,
    const VibratorCurveInterval &originalInterval, VibratorCurvePoint& point)
{
    static_assert(INTENSITY_UPPER_BOUND != INTENSITY_LOWER_BOUND, "upper bound and lower bound cannot be the same");
    point.frequency = RestrictFrequencyRange(modulationInterval.frequency + originalInterval.frequency);
    point.intensity = RestrictIntensityRange(
        modulationInterval.intensity * originalInterval.intensity / (INTENSITY_UPPER_BOUND - INTENSITY_LOWER_BOUND));
}

int32_t VibratorServiceClient::RestrictFrequencyRange(int32_t frequency)
{
    if (frequency > FREQUENCY_UPPER_BOUND) {
        return FREQUENCY_UPPER_BOUND;
    } else if (frequency < FREQUENCY_LOWER_BOUND) {
        return FREQUENCY_LOWER_BOUND;
    } else {
        return frequency;
    }
}

int32_t VibratorServiceClient::RestrictIntensityRange(int32_t intensity)
{
    if (intensity > INTENSITY_UPPER_BOUND) {
        return INTENSITY_UPPER_BOUND;
    } else if (intensity < INTENSITY_LOWER_BOUND) {
        return INTENSITY_LOWER_BOUND;
    } else {
        return intensity;
    }
}

bool VibratorServiceClient::SkipEventAndConvertVibratorEvent(const VibratorEvent &vibratorEvent,
    VibratePattern &vibratePattern, int32_t patternStartTime, VibrateEvent &vibrateEvent)
{
    int32_t eventStartTime = vibratorEvent.time + patternStartTime;
    if (vibratePattern.startTime > eventStartTime) {
        if (vibratorEvent.type == EVENT_TYPE_CONTINUOUS &&
            (eventStartTime + vibratorEvent.duration - vibratePattern.startTime) >= MIN_VIBRATOR_EVENT_TIME) {
            vibrateEvent.tag = static_cast<VibrateTag>(vibratorEvent.type);
            vibrateEvent.duration = eventStartTime + vibratorEvent.duration - vibratePattern.startTime;
            vibrateEvent.intensity = vibratorEvent.intensity;
            vibrateEvent.frequency = vibratorEvent.frequency;
            vibrateEvent.index = vibratorEvent.index;
            vibratePattern.patternDuration += vibrateEvent.duration;
            vibratePattern.events.emplace_back(vibrateEvent);
        }
        return true;
    }
    return false;
}

int32_t VibratorServiceClient::GetVibratorList(const VibratorIdentifier& identifier,
    std::vector<VibratorInfos>& vibratorInfo)
{
    CALL_LOG_ENTER;
    MISC_HILOGD("VibratorIdentifier = [deviceId = %{public}d, vibratorId = %{public}d]", identifier.deviceId,
        identifier.vibratorId);
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    CHKPR(miscdeviceProxy_, OHOS::Sensors::ERROR);

    VibratorIdentifierIPC param;
    param.deviceId = identifier.deviceId;
    param.vibratorId = identifier.vibratorId;
    std::vector<VibratorInfoIPC> vibratorInfoList;
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "GetVibratorList");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = miscdeviceProxy_->GetVibratorList(param, vibratorInfoList);
    WriteOtherHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_GET_VIBRATOR_LIST, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGE("Get vibrator info list failed, [ret = %{public}d, deviceId = %{public}d,\
            vibratorId = %{public}d]", ret, identifier.deviceId, identifier.vibratorId);
        return ret;
    }
    for (auto &info : vibratorInfoList) {
        VibratorInfos resInfo;
        resInfo.deviceId = info.deviceId;
        resInfo.vibratorId = info.vibratorId;
        resInfo.deviceName = info.deviceName;
        resInfo.isSupportHdHaptic = info.isSupportHdHaptic;
        resInfo.isLocalVibrator = info.isLocalVibrator;
        vibratorInfo.push_back(resInfo);
    }
    return OHOS::Sensors::SUCCESS;
}

int32_t VibratorServiceClient::GetEffectInfo(const VibratorIdentifier& identifier,
    const std::string& effectType, EffectInfo& effectInfo)
{
    CALL_LOG_ENTER;
    MISC_HILOGD("VibratorIdentifier = [deviceId = %{public}d, vibratorId = %{public}d, effectType = %{public}s]",
        identifier.deviceId, identifier.vibratorId, effectType.c_str());
    int32_t ret = InitServiceClient();
    if (ret != OHOS::Sensors::SUCCESS) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    CHKPR(miscdeviceProxy_, OHOS::Sensors::ERROR);
    VibratorIdentifierIPC param;
    param.deviceId = identifier.deviceId;
    param.vibratorId = identifier.vibratorId;
    EffectInfoIPC resInfo;
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "GetEffectInfo");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = miscdeviceProxy_->GetEffectInfo(param, effectType, resInfo);
    WriteOtherHiSysIPCEvent(IMiscdeviceServiceIpcCode::COMMAND_GET_EFFECT_INFO, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        MISC_HILOGE("Get effect info failed, [ret = %{public}d, deviceId = %{public}d, vibratorId = %{public}d,\
            effectType = %{public}s]", ret, identifier.deviceId, identifier.vibratorId, effectType.c_str());
        return ret;
    }
    effectInfo.isSupportEffect = resInfo.isSupportEffect;
    return OHOS::Sensors::SUCCESS;
}

void VibratorServiceClient::WriteVibratorHiSysIPCEvent(IMiscdeviceServiceIpcCode code, int32_t ret)
{
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
    if (ret != NO_ERROR) {
        switch (code) {
            case IMiscdeviceServiceIpcCode::COMMAND_VIBRATE:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "Vibrate", "ERROR_CODE", ret);
                break;
            case IMiscdeviceServiceIpcCode::COMMAND_STOP_VIBRATOR:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "StopVibrator", "ERROR_CODE", ret);
                break;
            case IMiscdeviceServiceIpcCode::COMMAND_PLAY_VIBRATOR_EFFECT:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "PlayVibratorEffect", "ERROR_CODE", ret);
                break;
            case IMiscdeviceServiceIpcCode::COMMAND_STOP_VIBRATOR_BY_MODE:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "StopVibratorByMode", "ERROR_CODE", ret);
                break;
            case IMiscdeviceServiceIpcCode::COMMAND_PLAY_VIBRATOR_CUSTOM:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "PlayVibratorCustom", "ERROR_CODE", ret);
                break;
            case IMiscdeviceServiceIpcCode::COMMAND_GET_VIBRATOR_CAPACITY:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "GetVibratorCapacity", "ERROR_CODE", ret);
                break;
            case IMiscdeviceServiceIpcCode::COMMAND_GET_VIBRATOR_LIST:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "GetVibratorList", "ERROR_CODE", ret);
                break;
            case IMiscdeviceServiceIpcCode::COMMAND_GET_EFFECT_INFO:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "GetEffectInfo", "ERROR_CODE", ret);
                break;
            case IMiscdeviceServiceIpcCode::COMMAND_PLAY_PACKAGE_BY_SESSION_ID:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "PlayPackageBySessionId", "ERROR_CODE", ret);
                break;
            case IMiscdeviceServiceIpcCode::COMMAND_STOP_VIBRATE_BY_SESSION_ID:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "StopVibrateBySessionId", "ERROR_CODE", ret);
                break;
            default:
                MISC_HILOGW("Code does not exist, code:%{public}d", static_cast<int32_t>(code));
                break;
        }
    }
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
}

void VibratorServiceClient::WriteOtherHiSysIPCEvent(IMiscdeviceServiceIpcCode code, int32_t ret)
{
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
    if (ret != NO_ERROR) {
        switch (code) {
            case IMiscdeviceServiceIpcCode::COMMAND_IS_SUPPORT_EFFECT:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "IsSupportEffect", "ERROR_CODE", ret);
                break;
            case IMiscdeviceServiceIpcCode::COMMAND_GET_DELAY_TIME:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "GetDelayTime", "ERROR_CODE", ret);
                break;
            case IMiscdeviceServiceIpcCode::COMMAND_PLAY_PATTERN:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "PlayPattern", "ERROR_CODE", ret);
                break;
            case IMiscdeviceServiceIpcCode::COMMAND_TRANSFER_CLIENT_REMOTE_OBJECT:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "TransferClientRemoteObject", "ERROR_CODE", ret);
                break;
            case IMiscdeviceServiceIpcCode::COMMAND_PLAY_PRIMITIVE_EFFECT:
                HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
                    HiSysEvent::EventType::FAULT, "PKG_NAME", "PlayPrimitiveEffect", "ERROR_CODE", ret);
                break;
            default:
                MISC_HILOGW("Code does not exist, code:%{public}d", static_cast<int32_t>(code));
                break;
        }
    }
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
}
} // namespace Sensors
} // namespace OHOS
