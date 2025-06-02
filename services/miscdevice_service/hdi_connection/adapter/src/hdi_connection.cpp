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
#include "hdi_connection.h"

#include <thread>

#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif // HIVIEWDFX_HISYSEVENT_ENABLE

#include "sensors_errors.h"
#include "vibrator_plug_callback.h"

#ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
using OHOS::HDI::Vibrator::V2_0::DeviceVibratorInfo;
#endif // HDF_DRIVERS_INTERFACE_VIBRATOR

#undef LOG_TAG
#define LOG_TAG "HdiConnection"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
sptr<IVibratorPlugCallback> g_eventCallback = nullptr;
constexpr int32_t GET_HDI_SERVICE_COUNT = 10;
constexpr uint32_t WAIT_MS = 100;
} // namespace
DevicePlugCallback HdiConnection::devicePlugCb_ = nullptr;

int32_t HdiConnection::ConnectHdi()
{
    CALL_LOG_ENTER;
    int32_t retry = 0;
    while (retry < GET_HDI_SERVICE_COUNT) {
        vibratorInterface_ = IVibratorInterface::Get();
        if (vibratorInterface_ != nullptr) {
            MISC_HILOGW("Connect v2_0 hdi success");
            g_eventCallback = new (std::nothrow) VibratorPlugCallback();
            CHKPR(g_eventCallback, ERR_NO_INIT);
            RegisterHdiDeathRecipient();
            return ERR_OK;
        }
        retry++;
        MISC_HILOGW("Connect hdi service failed, retry:%{public}d", retry);
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
    }
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
    HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_HDF_SERVICE_EXCEPTION",
        HiSysEvent::EventType::FAULT, "PKG_NAME", "ConnectHdi", "ERROR_CODE", VIBRATOR_HDF_CONNECT_ERR);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
    MISC_HILOGE("Vibrator interface initialization failed");
    return ERR_INVALID_VALUE;
}

int32_t HdiConnection::StartOnce(const VibratorIdentifierIPC &identifier, uint32_t duration)
{
    CHKPR(vibratorInterface_, ERR_INVALID_VALUE);
    DeviceVibratorInfo deviceVibratorInfo = {
        .deviceId = identifier.deviceId,
        .vibratorId = identifier.vibratorId
    };
    int32_t ret = vibratorInterface_->StartOnce(deviceVibratorInfo, duration);
    if (ret < 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "StartOnce", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("StartOnce failed");
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::Start(const VibratorIdentifierIPC &identifier, const std::string &effectType)
{
    MISC_HILOGD("Time delay measurement:end time");
    if (effectType.empty()) {
        MISC_HILOGE("effectType is null");
        return VIBRATOR_ON_ERR;
    }
    CHKPR(vibratorInterface_, ERR_INVALID_VALUE);
    DeviceVibratorInfo deviceVibratorInfo = {
        .deviceId = identifier.deviceId,
        .vibratorId = identifier.vibratorId
    };
    int32_t ret = vibratorInterface_->Start(deviceVibratorInfo, effectType);
    if (ret < 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "Start", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("Start failed");
        return ret;
    }
    return ERR_OK;
}

#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
int32_t HdiConnection::EnableCompositeEffect(const VibratorIdentifierIPC &identifier,
    const HdfCompositeEffect &hdfCompositeEffect)
{
    MISC_HILOGD("Time delay measurement:end time");
    if (hdfCompositeEffect.compositeEffects.empty()) {
        MISC_HILOGE("compositeEffects is empty");
        return VIBRATOR_ON_ERR;
    }
    CHKPR(vibratorInterface_, ERR_INVALID_VALUE);
    DeviceVibratorInfo deviceVibratorInfo = {
        .deviceId = identifier.deviceId,
        .vibratorId = identifier.vibratorId
    };
    int32_t ret = vibratorInterface_->EnableCompositeEffect(deviceVibratorInfo, hdfCompositeEffect);
    if (ret < 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "EnableCompositeEffect", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("EnableCompositeEffect failed");
        return ret;
    }
    return ERR_OK;
}

bool HdiConnection::IsVibratorRunning(const VibratorIdentifierIPC &identifier)
{
    bool state = false;
    CHKPR(vibratorInterface_, false);
    DeviceVibratorInfo deviceVibratorInfo = {
        .deviceId = identifier.deviceId,
        .vibratorId = identifier.vibratorId
    };
    vibratorInterface_->IsVibratorRunning(deviceVibratorInfo, state);
    return state;
}
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM

std::optional<HdfEffectInfo> HdiConnection::GetEffectInfo(const VibratorIdentifierIPC &identifier,
    const std::string &effect)
{
    if (vibratorInterface_ == nullptr) {
        MISC_HILOGE("Connect v2_0 hdi failed");
        return std::nullopt;
    }
    DeviceVibratorInfo deviceVibratorInfo = {
        .deviceId = identifier.deviceId,
        .vibratorId = identifier.vibratorId
    };
    HdfEffectInfo effectInfo;
    int32_t ret = vibratorInterface_->GetEffectInfo(deviceVibratorInfo, effect, effectInfo);
    if (ret < 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "GetEffectInfo", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("GetEffectInfo failed");
        return std::nullopt;
    }
    return effectInfo;
}

int32_t HdiConnection::Stop(const VibratorIdentifierIPC &identifier, HdfVibratorMode mode)
{
    CHKPR(vibratorInterface_, ERR_INVALID_VALUE);
    DeviceVibratorInfo deviceVibratorInfo = {
        .deviceId = identifier.deviceId,
        .vibratorId = identifier.vibratorId
    };
    int32_t ret = vibratorInterface_->Stop(deviceVibratorInfo, mode);
    if (ret < 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "Stop", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("Stop failed");
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::GetDelayTime(const VibratorIdentifierIPC &identifier, int32_t mode, int32_t &delayTime)
{
    CHKPR(vibratorInterface_, ERR_INVALID_VALUE);
    DeviceVibratorInfo deviceVibratorInfo = {
        .deviceId = identifier.deviceId,
        .vibratorId = identifier.vibratorId
    };
    int32_t ret = vibratorInterface_->GetHapticStartUpTime(deviceVibratorInfo, mode, delayTime);
    if (ret < 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "GetDelayTime", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("GetDelayTime failed");
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::GetVibratorCapacity(const VibratorIdentifierIPC &identifier, VibratorCapacity &capacity)
{
    CHKPR(vibratorInterface_, ERR_INVALID_VALUE);
    HapticCapacity hapticCapacity;
    DeviceVibratorInfo deviceVibratorInfo = {
        .deviceId = identifier.deviceId,
        .vibratorId = identifier.vibratorId
    };
    int32_t ret = vibratorInterface_->GetHapticCapacity(deviceVibratorInfo, hapticCapacity);
    if (ret < 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "GetVibratorCapacity", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("GetVibratorCapacity failed");
        return ret;
    }
    capacity.isSupportHdHaptic = hapticCapacity.isSupportHdHaptic;
    capacity.isSupportPresetMapping = hapticCapacity.isSupportPresetMapping;
    capacity.isSupportTimeDelay = hapticCapacity.isSupportTimeDelay;
    capacity.Dump();
    return ERR_OK;
}

int32_t HdiConnection::PlayPattern(const VibratorIdentifierIPC &identifier, const VibratePattern &pattern)
{
    CHKPR(vibratorInterface_, ERR_INVALID_VALUE);
    HapticPaket packet = {};
    packet.time = pattern.startTime;
    int32_t eventNum = static_cast<int32_t>(pattern.events.size());
    packet.eventNum = eventNum;
    for (int32_t i = 0; i < eventNum; ++i) {
        HapticEvent hapticEvent = {};
        hapticEvent.type = static_cast<EVENT_TYPE>(pattern.events[i].tag);
        hapticEvent.time = pattern.events[i].time;
        hapticEvent.duration = pattern.events[i].duration;
        hapticEvent.intensity = pattern.events[i].intensity;
        hapticEvent.frequency = pattern.events[i].frequency;
        hapticEvent.index = pattern.events[i].index;
        int32_t pointNum = static_cast<int32_t>(pattern.events[i].points.size());
        hapticEvent.pointNum = pointNum;
        for (int32_t j = 0; j < pointNum; ++j) {
            CurvePoint hapticPoint = {};
            hapticPoint.time = pattern.events[i].points[j].time;
            hapticPoint.intensity = pattern.events[i].points[j].intensity;
            hapticPoint.frequency = pattern.events[i].points[j].frequency;
            hapticEvent.points.emplace_back(hapticPoint);
        }
        packet.events.emplace_back(hapticEvent);
    }
    DeviceVibratorInfo deviceVibratorInfo = {
        .deviceId = identifier.deviceId,
        .vibratorId = identifier.vibratorId
    };
    int32_t ret = vibratorInterface_->PlayHapticPattern(deviceVibratorInfo, packet);
    if (ret < 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "PlayHapticPattern", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("PlayHapticPattern failed");
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::DestroyHdiConnection()
{
    CALL_LOG_ENTER;
    CHKPR(vibratorInterface_, ERR_NO_INIT);
    int32_t ret = vibratorInterface_->UnRegVibratorPlugCallback(g_eventCallback);
    if (ret != 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "DestroyHdiConnection", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("UnRegVibratorPlugCallback is failed");
        return ret;
    }
    g_eventCallback = nullptr;
    UnregisterHdiDeathRecipient();
    return ERR_OK;
}

void HdiConnection::RegisterHdiDeathRecipient()
{
    CALL_LOG_ENTER;
    if (vibratorInterface_ == nullptr) {
        MISC_HILOGE("Connect v2_0 hdi failed");
        return;
    }
    if (hdiDeathObserver_ == nullptr) {
        hdiDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<HdiConnection *>(this));
        CHKPV(hdiDeathObserver_);
    }
    OHOS::HDI::hdi_objcast<IVibratorInterface>(vibratorInterface_)->AddDeathRecipient(hdiDeathObserver_);
}

void HdiConnection::UnregisterHdiDeathRecipient()
{
    CALL_LOG_ENTER;
    if (vibratorInterface_ == nullptr || hdiDeathObserver_ == nullptr) {
        MISC_HILOGE("vibratorInterface_ or hdiDeathObserver_ is null");
        return;
    }
    OHOS::HDI::hdi_objcast<IVibratorInterface>(vibratorInterface_)->RemoveDeathRecipient(hdiDeathObserver_);
}

void HdiConnection::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    CALL_LOG_ENTER;
    sptr<IRemoteObject> hdiService = object.promote();
    if (hdiService == nullptr || hdiDeathObserver_ == nullptr) {
        MISC_HILOGE("Invalid remote object or hdiDeathObserver_ is null");
        return;
    }
    hdiService->RemoveDeathRecipient(hdiDeathObserver_);
    Reconnect();
}

void HdiConnection::Reconnect()
{
    int32_t ret = ConnectHdi();
    if (ret != ERR_OK) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "Reconnect", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("Connect hdi fail");
    }
}

int32_t HdiConnection::StartByIntensity(const VibratorIdentifierIPC &identifier, const std::string &effect,
    int32_t intensity)
{
    MISC_HILOGD("Time delay measurement:end time, effect:%{public}s, intensity:%{public}d", effect.c_str(), intensity);
    if (effect.empty()) {
        MISC_HILOGE("effect is null");
        return VIBRATOR_ON_ERR;
    }
    CHKPR(vibratorInterface_, ERR_INVALID_VALUE);
    DeviceVibratorInfo deviceVibratorInfo = {
        .deviceId = identifier.deviceId,
        .vibratorId = identifier.vibratorId
    };
    int32_t ret = vibratorInterface_->StartByIntensity(deviceVibratorInfo, effect, intensity);
    if (ret < 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "StartByIntensity", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("StartByIntensity failed");
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::GetVibratorInfo(std::vector<HdfVibratorInfo> &hdfVibratorInfo)
{
    CHKPR(vibratorInterface_, ERR_INVALID_VALUE);
    int32_t ret = vibratorInterface_->GetVibratorInfo(hdfVibratorInfo);
    if (ret != ERR_OK) {
        MISC_HILOGE("GetVibratorInfo failed");
    }
    return ret;
}

int32_t HdiConnection::GetAllWaveInfo(const VibratorIdentifierIPC &identifier,
    std::vector<HdfWaveInformation> &waveInfos)
{
    CHKPR(vibratorInterface_, ERR_INVALID_VALUE);
    DeviceVibratorInfo deviceVibratorInfo = {
        .deviceId = identifier.deviceId,
        .vibratorId = identifier.vibratorId
    };
    int32_t ret = vibratorInterface_->GetAllWaveInfo(deviceVibratorInfo, waveInfos);
    if (ret != ERR_OK) {
        MISC_HILOGE("GetAllWaveInfo failed");
    }
    return ret;
}

int32_t HdiConnection::GetVibratorList(const VibratorIdentifierIPC &identifier,
    std::vector<HdfVibratorInfo> &vibratorInfo)
{
    CHKPR(vibratorInterface_, ERR_INVALID_VALUE);
    DeviceVibratorInfo deviceVibratorInfo = {
        .deviceId = identifier.deviceId,
        .vibratorId = identifier.vibratorId
    };
    int32_t ret = vibratorInterface_->GetDeviceVibratorInfo(deviceVibratorInfo, vibratorInfo);
    if (ret != ERR_OK) {
        MISC_HILOGE("GetVibratorList failed");
    }
    return ret;
}

int32_t HdiConnection::GetEffectInfo(const VibratorIdentifierIPC &identifier, const std::string &effectType,
    HdfEffectInfo &effectInfo)
{
    CHKPR(vibratorInterface_, ERR_INVALID_VALUE);
    DeviceVibratorInfo deviceVibratorInfo = {
        .deviceId = identifier.deviceId,
        .vibratorId = identifier.vibratorId
    };
    int32_t ret = vibratorInterface_->GetEffectInfo(deviceVibratorInfo, effectType, effectInfo);
    if (ret != ERR_OK) {
        MISC_HILOGE("GetEffectInfo failed");
    }
    return ret;
}

int32_t HdiConnection::RegisterVibratorPlugCallback(DevicePlugCallback cb)
{
    CALL_LOG_ENTER;
    CHKPR(cb, ERR_NO_INIT);
    CHKPR(vibratorInterface_, ERR_NO_INIT);
    devicePlugCb_ = cb;
    int32_t ret = vibratorInterface_->RegVibratorPlugCallback(g_eventCallback);
    if (ret != 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "RegisterVibratorCallback", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("RegVibratorPlugCallback is failed");
        return ret;
    }
    return ERR_OK;
}

DevicePlugCallback HdiConnection::GetVibratorPlugCb()
{
    if (devicePlugCb_ == nullptr) {
        MISC_HILOGE("GetVibratorPlugCb cannot be null");
    }
    return devicePlugCb_;
}
}  // namespace Sensors
}  // namespace OHOS