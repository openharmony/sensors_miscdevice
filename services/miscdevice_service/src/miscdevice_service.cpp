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

#include "miscdevice_service.h"

#ifdef OHOS_BUILD_ENABLE_VIBRATOR_INPUT_METHOD
#include "common_event_support.h"
#endif // OHOS_BUILD_ENABLE_VIBRATOR_INPUT_METHOD
#include "death_recipient_template.h"
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
#ifdef MEMMGR_ENABLE
#include "mem_mgr_client.h"
#endif // MEMMGR_ENABLE
#include "system_ability_definition.h"

#include "sensors_errors.h"
#include "vibration_priority_manager.h"
#include "vibrator_client_proxy.h"

#ifdef HDF_DRIVERS_INTERFACE_LIGHT
#include "v1_0/light_interface_proxy.h"
#endif // HDF_DRIVERS_INTERFACE_LIGHT

#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
#include "parameter.h"
#include "parameters.h"
#include "permission_util.h"
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM

#undef LOG_TAG
#define LOG_TAG "MiscdeviceService"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
auto g_miscdeviceService = MiscdeviceDelayedSpSingleton<MiscdeviceService>::GetInstance();
const bool G_REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(g_miscdeviceService.GetRefPtr());
const std::string VIBRATE_PERMISSION = "ohos.permission.VIBRATE";
const std::string LIGHT_PERMISSION = "ohos.permission.SYSTEM_LIGHT_CONTROL";
constexpr int32_t MAX_LIGHT_COUNT = 0XFF;
constexpr int32_t MIN_VIBRATOR_TIME = 0;
constexpr int32_t MAX_VIBRATOR_TIME = 1800000;
constexpr int32_t MIN_VIBRATOR_COUNT = 1;
constexpr int32_t MAX_VIBRATOR_COUNT = 1000;
constexpr int32_t INTENSITY_MIN = 0;
constexpr int32_t INTENSITY_MAX = 100;
constexpr int32_t FREQUENCY_MIN = 0;
constexpr int32_t FREQUENCY_MAX = 100;
constexpr int32_t INTENSITY_ADJUST_MIN = 0;
constexpr int32_t INTENSITY_ADJUST_MAX = 100;
constexpr int32_t FREQUENCY_ADJUST_MIN = -100;
constexpr int32_t FREQUENCY_ADJUST_MAX = 100;
constexpr int32_t INVALID_PID = -1;
constexpr int32_t BASE_YEAR = 1900;
constexpr int32_t BASE_MON = 1;
constexpr int32_t CONVERSION_RATE = 1000;
constexpr int32_t HOURS_IN_DAY = 24;
constexpr int32_t MINUTES_IN_HOUR = 60;
constexpr int32_t SECONDS_IN_MINUTE = 60;
constexpr uint32_t MAX_SUPPORT_CLIENT_NUM = 1024;
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
const std::string PHONE_TYPE = "phone";
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_PRESET_INFO
constexpr int32_t SHORT_VIBRATOR_DURATION = 50;
#endif // OHOS_BUILD_ENABLE_VIBRATOR_PRESET_INFO
constexpr int32_t LOG_COUNT_FIVE = 5;
const inline char *DEVICE_MUTE_FLAG = "vendor.device.vibrator.mute";
}  // namespace

std::atomic_int32_t MiscdeviceService::timeModeCallTimes_ = 0;
std::atomic_int32_t MiscdeviceService::presetModeCallTimes_ = 0;
std::atomic_int32_t MiscdeviceService::fileModeCallTimes_ = 0;
std::atomic_int32_t MiscdeviceService::patternModeCallTimes_ = 0;
std::atomic_bool MiscdeviceService::stop_ = false;
std::unordered_map<std::string, InvalidVibratorInfo> MiscdeviceService::invalidVibratorInfoMap_;
std::mutex MiscdeviceService::invalidVibratorInfoMutex_;
std::mutex MiscdeviceService::stopMutex_;
bool MiscdeviceService::isVibrationPriorityReady_ = false;
std::map<int32_t, VibratorAllInfos> MiscdeviceService::devicesManageMap_;
std::map<sptr<IRemoteObject>, int32_t> MiscdeviceService::clientPidMap_;
std::atomic_bool MiscdeviceService::deviceMute_ = false;

MiscdeviceService::MiscdeviceService()
    : SystemAbility(MISCDEVICE_SERVICE_ABILITY_ID, true),
      lightExist_(false),
      vibratorExist_(false),
      state_(MiscdeviceServiceState::STATE_STOPPED)
{
    MISC_HILOGD("Add SystemAbility");
}

MiscdeviceService::~MiscdeviceService()
{
    if (reportCallTimesThread_.joinable()) {
        stop_ = true;
        stopCondition_.notify_all();
        reportCallTimesThread_.join();
    }
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    RemoveParameterWatcher(DEVICE_MUTE_FLAG, ParameterCallback, this);
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
}

void MiscdeviceService::OnDump()
{
    MISC_HILOGI("Ondump is invoked");
}

int32_t MiscdeviceService::SubscribeCommonEvent(const std::string &eventName,
    EventReceiver receiver) __attribute__((no_sanitize("cfi")))
{
    if (receiver == nullptr) {
        MISC_HILOGE("receiver is nullptr");
        return ERROR;
    }
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(eventName);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    auto subscribePtr = std::make_shared<MiscdeviceCommonEventSubscriber>(subscribeInfo, receiver);
    if (!EventFwk::CommonEventManager::SubscribeCommonEvent(subscribePtr)) {
        MISC_HILOGE("Subscribe common event fail");
        return ERROR;
    }
    return ERR_OK;
}

void MiscdeviceService::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    MISC_HILOGI("OnAddSystemAbility systemAbilityId:%{public}d", systemAbilityId);
    switch (systemAbilityId) {
        case MEMORY_MANAGER_SA_ID: {
            MISC_HILOGI("Memory manager service start");
#ifdef MEMMGR_ENABLE
            Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(),
                PROCESS_TYPE_SA, PROCESS_STATUS_STARTED, MISCDEVICE_SERVICE_ABILITY_ID);
#endif // MEMMGR_ENABLE
            break;
        }
        case COMMON_EVENT_SERVICE_ID: {
            MISC_HILOGI("Common event service start");
            int32_t ret = SubscribeCommonEvent("usual.event.DATA_SHARE_READY",
                [this](const EventFwk::CommonEventData &data) { this->OnReceiveEvent(data); });
            if (ret != ERR_OK) {
                MISC_HILOGE("Subscribe usual.event.DATA_SHARE_READY fail");
            }
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_INPUT_METHOD
            ret = SubscribeCommonEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED,
                [this](const EventFwk::CommonEventData &data) { this->OnReceiveUserSwitchEvent(data); });
            if (ret != ERR_OK) {
                MISC_HILOGE("Subscribe usual.event.USER_SWITCHED fail");
            }
#endif // OHOS_BUILD_ENABLE_VIBRATOR_INPUT_METHOD
            AddSystemAbilityListener(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
            break;
        }
        case DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID: {
            MISC_HILOGI("Distributed kv data service start");
            std::lock_guard<std::mutex> lock(isVibrationPriorityReadyMutex_);
            if (isVibrationPriorityReady_) { /** Datashare will reconnect to the client and register data after alive */
                MISC_HILOGI("PriorityManager already init");
                break;
            }
            if (PriorityManager->Init()) {
                MISC_HILOGI("PriorityManager init");
                isVibrationPriorityReady_ = true;
            } else {
                MISC_HILOGE("PriorityManager init fail");
            }
            break;
        }
        default: {
            MISC_HILOGI("Unknown service, systemAbilityId:%{public}d", systemAbilityId);
            break;
        }
    }
}

void MiscdeviceService::OnReceiveEvent(const EventFwk::CommonEventData &data)
{
    const auto &want = data.GetWant();
    std::string action = want.GetAction();
    if (action == "usual.event.DATA_SHARE_READY") {
        MISC_HILOGI("On receive usual.event.DATA_SHARE_READY");
        std::lock_guard<std::mutex> lock(isVibrationPriorityReadyMutex_);
        if (isVibrationPriorityReady_) {
            MISC_HILOGI("PriorityManager already init");
            return;
        }
        if (PriorityManager->Init()) {
            MISC_HILOGI("PriorityManager init");
            isVibrationPriorityReady_ = true;
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
            std::call_once(isRegistered_, [this]() { RegisterDeviceMuteObserver(); });
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
        } else {
            MISC_HILOGE("PriorityManager init fail");
        }
    }
}

#ifdef OHOS_BUILD_ENABLE_VIBRATOR_INPUT_METHOD
void MiscdeviceService::OnReceiveUserSwitchEvent(const EventFwk::CommonEventData &data)
{
    const auto &want = data.GetWant();
    std::string action = want.GetAction();
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED) {
        MISC_HILOGI("OnReceiveUserSwitchEvent user switched");
        PriorityManager->ReregisterCurrentUserObserver();
    }
}
#endif // OHOS_BUILD_ENABLE_VIBRATOR_INPUT_METHOD

void MiscdeviceService::OnStart()
{
    CALL_LOG_ENTER;
    if (state_ == MiscdeviceServiceState::STATE_RUNNING) {
        MISC_HILOGW("state_ already started");
        return;
    }
    if (!InitInterface()) {
        MISC_HILOGE("Init interface error");
    }
    if (!InitLightInterface()) {
        MISC_HILOGE("InitLightInterface failed");
    }
    if (!SystemAbility::Publish(MiscdeviceDelayedSpSingleton<MiscdeviceService>::GetInstance())) {
        MISC_HILOGE("Publish MiscdeviceService failed");
        return;
    }
    std::lock_guard<std::mutex> lock(miscDeviceIdMapMutex_);
    auto ret = miscDeviceIdMap_.insert(std::make_pair(MiscdeviceDeviceId::LED, lightExist_));
    if (!ret.second) {
        MISC_HILOGI("Light exist in miscDeviceIdMap_");
        ret.first->second = lightExist_;
    }
    ret = miscDeviceIdMap_.insert(std::make_pair(MiscdeviceDeviceId::VIBRATOR, vibratorExist_));
    if (!ret.second) {
        MISC_HILOGI("Vibrator exist in miscDeviceIdMap_");
        ret.first->second = vibratorExist_;
    }
    state_ = MiscdeviceServiceState::STATE_RUNNING;
#ifdef MEMMGR_ENABLE
    AddSystemAbilityListener(MEMORY_MANAGER_SA_ID);
#endif // MEMMGR_ENABLE
    AddSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
    RegisterVibratorPlugCb();
    reportCallTimesThread_ = std::thread([this]() { this->ReportCallTimes(); });
}

int32_t MiscdeviceService::RegisterVibratorPlugCb()
{
    auto ret = vibratorHdiConnection_.RegisterVibratorPlugCallback(
        std::bind(&MiscdeviceService::SendMsgToClient, this, std::placeholders::_1));
    if (ret != ERR_OK) {
        MISC_HILOGE("RegisterVibratorPlugCallback failed");
        return false;
    }
    MISC_HILOGI("RegisterVibratorPlugCallback success");
    return true;
}

void MiscdeviceService::OnStartFuzz()
{
    CALL_LOG_ENTER;
    if (state_ == MiscdeviceServiceState::STATE_RUNNING) {
        MISC_HILOGW("state_ already started");
        return;
    }
    if (!InitInterface()) {
        MISC_HILOGE("Init interface error");
    }
    if (!InitLightInterface()) {
        MISC_HILOGE("InitLightInterface failed");
    }
    std::lock_guard<std::mutex> lock(miscDeviceIdMapMutex_);
    auto ret = miscDeviceIdMap_.insert(std::make_pair(MiscdeviceDeviceId::LED, lightExist_));
    if (!ret.second) {
        MISC_HILOGI("Light exist in miscDeviceIdMap_");
        ret.first->second = lightExist_;
    }
    ret = miscDeviceIdMap_.insert(std::make_pair(MiscdeviceDeviceId::VIBRATOR, vibratorExist_));
    if (!ret.second) {
        MISC_HILOGI("Vibrator exist in miscDeviceIdMap_");
        ret.first->second = vibratorExist_;
    }
    state_ = MiscdeviceServiceState::STATE_RUNNING;
}

bool MiscdeviceService::InitInterface()
{
    auto ret = vibratorHdiConnection_.ConnectHdi();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitVibratorServiceImpl failed");
        return false;
    }
    GetOnlineVibratorInfo();
    return true;
}

bool MiscdeviceService::InitLightInterface()
{
    auto ret = lightHdiConnection_.ConnectHdi();
    if (ret != ERR_OK) {
        MISC_HILOGE("ConnectHdi failed");
        return false;
    }
    return true;
}

bool MiscdeviceService::IsValid(int32_t lightId)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> lightInfosLock(lightInfosMutex_);
    for (const auto &item : lightInfos_) {
        if (lightId == item.GetLightId()) {
            return true;
        }
    }
    return false;
}

bool MiscdeviceService::IsLightAnimationValid(const LightAnimationIPC &animation)
{
    CALL_LOG_ENTER;
    int32_t mode = animation.GetMode();
    int32_t onTime = animation.GetOnTime();
    int32_t offTime = animation.GetOffTime();
    if ((mode < 0) || (mode >= LIGHT_MODE_BUTT)) {
        MISC_HILOGE("animation mode is invalid, mode:%{public}d", mode);
        return false;
    }
    if ((onTime < 0) || (offTime < 0)) {
        MISC_HILOGE("animation onTime or offTime is invalid, onTime:%{public}d, offTime:%{public}d",
            onTime,  offTime);
        return false;
    }
    return true;
}

void MiscdeviceService::OnStop()
{
    CALL_LOG_ENTER;
    if (state_ == MiscdeviceServiceState::STATE_STOPPED) {
        MISC_HILOGW("MiscdeviceService stopped already");
        return;
    }
    state_ = MiscdeviceServiceState::STATE_STOPPED;
    int32_t ret = vibratorHdiConnection_.DestroyHdiConnection();
    if (ret != ERR_OK) {
        MISC_HILOGE("Destroy hdi connection fail");
    }
#ifdef MEMMGR_ENABLE
    Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), PROCESS_TYPE_SA, PROCESS_STATUS_DIED,
        MISCDEVICE_SERVICE_ABILITY_ID);
#endif // MEMMGR_ENABLE
}

bool MiscdeviceService::ShouldIgnoreVibrate(const VibrateInfo &info, const VibratorIdentifierIPC& identifier)
{
    std::lock_guard<std::mutex> lock(isVibrationPriorityReadyMutex_);
    if (!isVibrationPriorityReady_) {
        MISC_HILOGE("Vibraion priority manager not ready");
        return VIBRATION;
    }
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    std::call_once(isRegistered_, [this]() { RegisterDeviceMuteObserver(); });
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    std::string curVibrateTime = GetCurrentTime();
    auto vibratorThread_ = GetVibratorThread(identifier);
    int32_t ret = PriorityManager->ShouldIgnoreVibrate(info, vibratorThread_, identifier);
    if (ret != VIBRATION) {
        MISC_HILOGE("ShouldIgnoreVibrate currentTime:%{public}s, ret:%{public}d", curVibrateTime.c_str(), ret);
    }
    return (ret != VIBRATION);
}

int32_t MiscdeviceService::Vibrate(const VibratorIdentifierIPC& identifier, int32_t timeOut, int32_t usage,
    bool systemUsage)
{
    timeModeCallTimes_ += 1;
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "VibrateStub", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("CheckVibratePermission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    if ((timeOut <= MIN_VIBRATOR_TIME) || (timeOut > MAX_VIBRATOR_TIME)
        || (usage >= USAGE_MAX) || (usage < 0)) {
        MISC_HILOGE("Invalid parameter");
        return PARAMETER_ERROR;
    }
    VibrateInfo info = {
        .mode = VIBRATE_TIME,
        .packageName = GetPackageName(GetCallingTokenID()),
        .pid = GetCallingPid(),
        .uid = GetCallingUid(),
        .usage = usage,
        .systemUsage = systemUsage,
        .duration = timeOut
    };
    std::string curVibrateTime = GetCurrentTime();
    if (StartVibrateThreadControl(identifier, info) != ERR_OK) {
        MISC_HILOGE("%{public}s:vibration is ignored and high priority is vibrating or no vibration found",
            curVibrateTime.c_str());
        return ERROR;
    }
    MISC_HILOGW("Start vibrator, currentTime:%{public}s, package:%{public}s, pid:%{public}d, usage:%{public}d,"
        "deviceId:%{public}d, vibratorId:%{public}d, duration:%{public}d", curVibrateTime.c_str(),
        info.packageName.c_str(), info.pid, info.usage, identifier.deviceId, identifier.vibratorId, info.duration);
    return NO_ERROR;
}

int32_t MiscdeviceService::StopVibrator(const VibratorIdentifierIPC& identifier)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "StopVibratorStub", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("Result:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    std::lock_guard<std::mutex> lock(devicesManageMutex_);
    return StopVibratorService(identifier);
}

int32_t MiscdeviceService::StopVibratorService(const VibratorIdentifierIPC& identifier)
{
    std::lock_guard<std::mutex> lock(vibratorThreadMutex_);
    std::vector<VibratorIdentifierIPC> result = CheckDeviceIdIsValid(identifier);
    size_t ignoreVibrateNum = 0;
    if (result.empty()) {
        MISC_HILOGD("result is empty, no need to stop");
        return ERROR;
    }
    for (const auto& paramIt : result) {
        auto vibratorThread_ = GetVibratorThread(paramIt);
        #if defined (OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM)
            if ((vibratorThread_ == nullptr) || (!vibratorThread_->IsRunning() &&
                !vibratorHdiConnection_.IsVibratorRunning(paramIt))) {
                MISC_HILOGD("Thread is not running, no need to stop");
                ignoreVibrateNum ++;
                continue;
            }
            if (vibratorHdiConnection_.IsVibratorRunning(paramIt)) {
                vibratorHdiConnection_.Stop(paramIt, HDF_VIBRATOR_MODE_PRESET);
                vibratorHdiConnection_.Stop(paramIt, HDF_VIBRATOR_MODE_HDHAPTIC);
            }
        #else
            if ((vibratorThread_ == nullptr) || (!vibratorThread_->IsRunning())) {
                MISC_HILOGD("Thread is not running, no need to stop");
                ignoreVibrateNum ++;
                continue;
            }
        #endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
            StopVibrateThread(vibratorThread_);
    }
    if (ignoreVibrateNum == result.size()) {
        MISC_HILOGD("No vibration, no need to stop");
        return NO_ERROR;
    }
    std::string packageName = GetPackageName(GetCallingTokenID());
    std::string curVibrateTime = GetCurrentTime();
    MISC_HILOGW("Stop vibrator, currentTime:%{public}s, package:%{public}s, pid:%{public}d, deviceId:%{public}d,"
        "vibratorId:%{public}d", curVibrateTime.c_str(), packageName.c_str(), GetCallingPid(), identifier.deviceId,
        identifier.vibratorId);
    return NO_ERROR;
}

int32_t MiscdeviceService::PlayVibratorEffect(const VibratorIdentifierIPC& identifier, const std::string &effect,
    int32_t count, int32_t usage, bool systemUsage)
{
    presetModeCallTimes_ += 1;
    int32_t checkResult = PlayVibratorEffectCheckAuthAndParam(count, usage);
    if (checkResult != ERR_OK) {
        MISC_HILOGE("CheckAuthAndParam failed, ret:%{public}d", checkResult);
        return checkResult;
    }
#ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
    std::optional<HdfEffectInfo> effectInfo = vibratorHdiConnection_.GetEffectInfo(identifier, effect);
    if (!effectInfo) {
        MISC_HILOGE("GetEffectInfo fail");
        return ERROR;
    }
    if (!(effectInfo->isSupportEffect)) {
        MISC_HILOGE("Effect not supported");
        return PARAMETER_ERROR;
    }
#endif // HDF_DRIVERS_INTERFACE_VIBRATOR
    VibrateInfo info = {
        .mode = VIBRATE_PRESET,
        .packageName = GetPackageName(GetCallingTokenID()),
        .pid = GetCallingPid(),
        .uid = GetCallingUid(),
        .usage = usage,
        .systemUsage = systemUsage,
#ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
        .duration = effectInfo->duration,
#endif // HDF_DRIVERS_INTERFACE_VIBRATOR
        .effect = effect,
        .count = count,
        .intensity = INTENSITY_ADJUST_MAX
    };
    std::string curVibrateTime = GetCurrentTime();
    if (StartVibrateThreadControl(identifier, info) != ERR_OK) {
        MISC_HILOGE("%{public}s:vibration is ignored and high priority is vibrating or no vibration found",
            curVibrateTime.c_str());
        return ERROR;
    }
    MISC_HILOGW("Start vibrator, currentTime:%{public}s, package:%{public}s, pid:%{public}d, usage:%{public}d,"
        "deviceId:%{public}d, vibratorId:%{public}d, duration:%{public}d, effect:%{public}s, count:%{public}d",
        curVibrateTime.c_str(), info.packageName.c_str(), info.pid, info.usage, identifier.deviceId,
        identifier.vibratorId, info.duration, info.effect.c_str(), info.count);
    return NO_ERROR;
}

int32_t MiscdeviceService::PlayVibratorEffectCheckAuthAndParam(int32_t count, int32_t usage)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "PlayVibratorEffectStub", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("CheckVibratePermission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    if ((count < MIN_VIBRATOR_COUNT) || (count > MAX_VIBRATOR_COUNT) || (usage >= USAGE_MAX) || (usage < 0)) {
        MISC_HILOGE("Invalid parameter");
        return PARAMETER_ERROR;
    }
    return ERR_OK;
}

void MiscdeviceService::StartVibrateThread(VibrateInfo info, const VibratorIdentifierIPC& identifier)
{
    auto vibratorThread_ = GetVibratorThread(identifier);
    if (vibratorThread_ == nullptr) {
        MISC_HILOGD("No effective vibrator thread");
        return;
    }
    std::vector<HdfWaveInformation> waveInfo;
    if (GetAllWaveInfo(identifier, waveInfo) != ERR_OK) {
        MISC_HILOGE("GetAllWaveInfo failed");
        return;
    }
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_PRESET_INFO
    VibrateInfo currentVibrateInfo = vibratorThread_->GetCurrentVibrateInfo();
    if (info.duration <= SHORT_VIBRATOR_DURATION && currentVibrateInfo.duration <= SHORT_VIBRATOR_DURATION &&
        info.mode == VIBRATE_PRESET && currentVibrateInfo.mode == VIBRATE_PRESET && info.count == 1) {
        vibratorThread_->UpdateVibratorEffect(info, identifier, waveInfo);
        FastVibratorEffect(info, identifier);
    } else {
#endif // OHOS_BUILD_ENABLE_VIBRATOR_PRESET_INFO
    StopVibrateThread(vibratorThread_);
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    if (vibratorHdiConnection_.IsVibratorRunning(identifier)) {
        vibratorHdiConnection_.Stop(identifier, HDF_VIBRATOR_MODE_PRESET);
        vibratorHdiConnection_.Stop(identifier, HDF_VIBRATOR_MODE_HDHAPTIC);
    }
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    vibratorThread_->UpdateVibratorEffect(info, identifier, waveInfo);
    vibratorThread_->Start("VibratorThread");
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_PRESET_INFO
    }
#endif // OHOS_BUILD_ENABLE_VIBRATOR_PRESET_INFO
    DumpHelper->SaveVibrateRecord(info);
}

void MiscdeviceService::StopVibrateThread(std::shared_ptr<VibratorThread> vibratorThread)
{
    if ((vibratorThread != nullptr) && (vibratorThread->IsRunning())) {
        vibratorThread->SetExitStatus(true);
        vibratorThread->WakeUp();
        vibratorThread->NotifyExitSync();
        vibratorThread->SetExitStatus(false);
        vibratorThread->ResetVibrateInfo();
    }
}

int32_t MiscdeviceService::StopVibratorByMode(const VibratorIdentifierIPC& identifier, const std::string &mode)
{
    std::lock_guard<std::mutex> lock(vibratorThreadMutex_);
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "StopVibratorByModeStub", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("CheckVibratePermission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    std::vector<VibratorIdentifierIPC> result = CheckDeviceIdIsValid(identifier);
    size_t ignoreVibrateNum = 0;
    if (result.empty()) {
        MISC_HILOGD("result is empty, no need to stop");
        return ERROR;
    }
    for (const auto& paramIt : result) {
        auto vibratorThread_ = GetVibratorThread(paramIt);
        if ((vibratorThread_ == nullptr) || (!vibratorThread_->IsRunning() &&
            !vibratorHdiConnection_.IsVibratorRunning(paramIt))) {
            MISC_HILOGD("Thread is not running, no need to stop");
            ignoreVibrateNum ++;
            continue;
        }
        const VibrateInfo info = vibratorThread_->GetCurrentVibrateInfo();
        if (info.mode != mode) {
            MISC_HILOGD("Stop vibration information mismatch");
            continue;
        }
        StopVibrateThread(vibratorThread_);
        if (vibratorHdiConnection_.IsVibratorRunning(paramIt)) {
            vibratorHdiConnection_.Stop(paramIt, HDF_VIBRATOR_MODE_PRESET);
        }
    }
    if (ignoreVibrateNum == result.size()) {
        MISC_HILOGD("No vibration, no need to stop");
        return NO_ERROR;
    }
    std::string packageName = GetPackageName(GetCallingTokenID());
    std::string curVibrateTime = GetCurrentTime();
    MISC_HILOGW("Stop vibrator, currentTime:%{public}s, package:%{public}s, pid:%{public}d, deviceId:%{public}d,"
        "vibratorId:%{public}d, mode:%{public}s", curVibrateTime.c_str(), packageName.c_str(), GetCallingPid(),
        identifier.deviceId, identifier.vibratorId, mode.c_str());
    return NO_ERROR;
}

int32_t MiscdeviceService::IsSupportEffect(const VibratorIdentifierIPC& identifier, const std::string &effect,
    bool &state)
{
#ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
    std::optional<HdfEffectInfo> effectInfo = vibratorHdiConnection_.GetEffectInfo(identifier, effect);
    if (!effectInfo) {
        MISC_HILOGE("GetEffectInfo fail");
        return ERROR;
    }
    state = effectInfo->isSupportEffect;
    std::string packageName = GetPackageName(GetCallingTokenID());
    std::string curVibrateTime = GetCurrentTime();
    MISC_HILOGI("IsSupportEffect, currentTime:%{public}s, package:%{public}s, pid:%{public}d, effect:%{public}s,"
        "state:%{public}d", curVibrateTime.c_str(), packageName.c_str(), GetCallingPid(), effect.c_str(), state);
#endif // HDF_DRIVERS_INTERFACE_VIBRATOR
    return NO_ERROR;
}

std::string MiscdeviceService::GetCurrentTime()
{
    timespec curTime;
    clock_gettime(CLOCK_REALTIME, &curTime);
    struct tm *timeinfo = localtime(&(curTime.tv_sec));
    std::string currentTime;
    if (timeinfo == nullptr) {
        MISC_HILOGE("timeinfo is null");
        return currentTime;
    }
    currentTime.append(std::to_string(timeinfo->tm_year + BASE_YEAR)).append("-")
        .append(std::to_string(timeinfo->tm_mon + BASE_MON)).append("-").append(std::to_string(timeinfo->tm_mday))
        .append(" ").append(std::to_string(timeinfo->tm_hour)).append(":").append(std::to_string(timeinfo->tm_min))
        .append(":").append(std::to_string(timeinfo->tm_sec)).append(".")
        .append(std::to_string(curTime.tv_nsec / (CONVERSION_RATE * CONVERSION_RATE)));
    return currentTime;
}

#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
void MiscdeviceService::ParameterCallback(const char *key, const char *value, void *context)
{
    if ((key == nullptr) || (value == nullptr) || (context == nullptr)) {
        MISC_HILOGE("ParameterCallback return invalid param");
        return;
    }
    deviceMute_.store(OHOS::system::GetBoolParameter(DEVICE_MUTE_FLAG, false));
    PriorityManager->SetIgnoreSwitchStatus(deviceMute_.load());
    if (deviceMute_.load()) {
        VibratorIdentifierIPC identifier;
        identifier.deviceId = -1;
        identifier.vibratorId = -1;
        MiscdeviceService *service = reinterpret_cast<MiscdeviceService *>(context);
        int32_t ret = service->StopVibratorService(identifier);
        if (ret != ERR_OK) {
            MISC_HILOGE("StopVibrator failed,ret:%{public}d", ret);
        }
    }
    MISC_HILOGI("deviceMute is %{public}d", deviceMute_.load());
}

void MiscdeviceService::RegisterDeviceMuteObserver()
{
    CALL_LOG_ENTER;
    deviceMute_.store(OHOS::system::GetBoolParameter(DEVICE_MUTE_FLAG, false));
    PriorityManager->SetIgnoreSwitchStatus(deviceMute_.load());
    int32_t ret = WatchParameter(DEVICE_MUTE_FLAG, ParameterCallback, this);
    if (ret != SUCCESS) {
        MISC_HILOGE("WatchParameter fail");
    }
}

int32_t MiscdeviceService::PlayVibratorCustom(const VibratorIdentifierIPC& identifier, const VibratePackage &pkg,
    const CustomHapticInfoIPC& customHapticInfoIPC)
{
    fileModeCallTimes_ += 1;
    int32_t checkResult = CheckAuthAndParam(customHapticInfoIPC.usage, customHapticInfoIPC.parameter, identifier);
    if (checkResult != ERR_OK) {
        MISC_HILOGE("CheckAuthAndParam failed, ret:%{public}d", checkResult);
        return checkResult;
    }
    VibrateInfo info = {
        .packageName = GetPackageName(GetCallingTokenID()),
        .pid = GetCallingPid(),
        .uid = GetCallingUid(),
        .usage = customHapticInfoIPC.usage,
        .systemUsage = customHapticInfoIPC.systemUsage,
        .package = pkg,
    };
    MergeVibratorParmeters(customHapticInfoIPC.parameter, info.package);
    info.package.Dump();
    VibratorCapacity capacity;
    if (GetHapticCapacityInfo(identifier, capacity) != ERR_OK) {
        MISC_HILOGE("GetVibratorCapacity failed");
        return ERROR;
    }
    if (capacity.isSupportHdHaptic) {
        info.mode = VIBRATE_CUSTOM_HD;
    } else if (capacity.isSupportPresetMapping) {
        info.mode = VIBRATE_CUSTOM_COMPOSITE_EFFECT;
    } else if (capacity.isSupportTimeDelay) {
        info.mode = VIBRATE_CUSTOM_COMPOSITE_TIME;
    }
    std::string curVibrateTime = GetCurrentTime();
    if (StartVibrateThreadControl(identifier, info) != ERR_OK) {
        MISC_HILOGE("%{public}s:vibration is ignored and high priority is vibrating or no vibration found",
            curVibrateTime.c_str());
        return ERROR;
    }
    MISC_HILOGW("Start vibrator, currentTime:%{public}s, package:%{public}s, pid:%{public}d, usage:%{public}d,"
        "vibratorId:%{public}d, duration:%{public}d", curVibrateTime.c_str(), info.packageName.c_str(), info.pid,
        info.usage, identifier.vibratorId, pkg.packageDuration);
    return NO_ERROR;
}
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM

int32_t MiscdeviceService::CheckAuthAndParam(int32_t usage, const VibrateParameter &parameter,
    const VibratorIdentifierIPC& identifier)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "PlayVibratorCustomStub", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("CheckVibratePermission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    VibratorCapacity capacity;
    if (GetHapticCapacityInfo(identifier, capacity) != ERR_OK) {
        MISC_HILOGE("GetVibratorCapacity failed");
        return ERROR;
    }
    if (!(capacity.isSupportHdHaptic || capacity.isSupportPresetMapping || capacity.isSupportTimeDelay)) {
        MISC_HILOGE("The device does not support this operation");
        return IS_NOT_SUPPORTED;
    }
    if ((usage >= USAGE_MAX) || (usage < 0) || (!CheckVibratorParmeters(parameter))) {
        MISC_HILOGE("Invalid parameter, usage:%{public}d", usage);
        return PARAMETER_ERROR;
    }
    return ERR_OK;
}

std::string MiscdeviceService::GetPackageName(AccessTokenID tokenId)
{
    std::string packageName;
    int32_t tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    switch (tokenType) {
        case ATokenTypeEnum::TOKEN_HAP: {
            HapTokenInfo hapInfo;
            if (AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo) != 0) {
                MISC_HILOGE("Get hap token info fail");
                return {};
            }
            packageName = hapInfo.bundleName;
            break;
        }
        case ATokenTypeEnum::TOKEN_NATIVE:
        case ATokenTypeEnum::TOKEN_SHELL: {
            NativeTokenInfo tokenInfo;
            if (AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo) != 0) {
                MISC_HILOGE("Get native token info fail");
                return {};
            }
            packageName = tokenInfo.processName;
            break;
        }
        default: {
            MISC_HILOGW("Token type not match");
            break;
        }
    }
    return packageName;
}

int32_t MiscdeviceService::GetLightList(std::vector<LightInfoIPC> &lightInfoIpcList)
{
    std::lock_guard<std::mutex> lightInfosLock(lightInfosMutex_);
    std::string packageName = GetPackageName(GetCallingTokenID());
    MISC_HILOGI("GetLightList, package:%{public}s", packageName.c_str());
    int32_t ret = lightHdiConnection_.GetLightList(lightInfos_);
    if (ret != ERR_OK) {
        MISC_HILOGE("GetLightList failed, ret:%{public}d", ret);
    }
    size_t lightCount = lightInfos_.size();
    MISC_HILOGI("lightCount:%{public}zu", lightCount);
    if (lightCount > MAX_LIGHT_COUNT) {
        lightCount = MAX_LIGHT_COUNT;
    }
    for (size_t i = 0; i < lightCount; ++i) {
        lightInfoIpcList.push_back(lightInfos_[i]);
    }
    return ERR_OK;
}

int32_t MiscdeviceService::TurnOn(int32_t lightId, int32_t singleColor, const LightAnimationIPC &animation)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), LIGHT_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "LIGHT_PERMISSIONS_EXCEPTION", HiSysEvent::EventType::SECURITY,
            "PKG_NAME", "turnOnStub", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("CheckLightPermission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    LightColor color;
    color.singleColor = singleColor;
    std::string packageName = GetPackageName(GetCallingTokenID());
    MISC_HILOGI("TurnOn, package:%{public}s", packageName.c_str());
    if (!IsValid(lightId)) {
        MISC_HILOGE("lightId is invalid, lightId:%{public}d", lightId);
        return MISCDEVICE_NATIVE_SAM_ERR;
    }
    if (!IsLightAnimationValid(animation)) {
        MISC_HILOGE("animation is invalid");
        return MISCDEVICE_NATIVE_SAM_ERR;
    }
    ret = lightHdiConnection_.TurnOn(lightId, color, animation);
    if (ret != ERR_OK) {
        MISC_HILOGE("TurnOn failed, error:%{public}d", ret);
        return ERROR;
    }
    return ret;
}

int32_t MiscdeviceService::TurnOff(int32_t lightId)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), LIGHT_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "LIGHT_PERMISSIONS_EXCEPTION", HiSysEvent::EventType::SECURITY,
            "PKG_NAME", "TurnOffStub", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("CheckLightPermission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    std::string packageName = GetPackageName(GetCallingTokenID());
    MISC_HILOGI("TurnOff, package:%{public}s", packageName.c_str());
    if (!IsValid(lightId)) {
        MISC_HILOGE("lightId is invalid, lightId:%{public}d", lightId);
        return MISCDEVICE_NATIVE_SAM_ERR;
    }
    ret = lightHdiConnection_.TurnOff(lightId);
    if (ret != ERR_OK) {
        MISC_HILOGE("TurnOff failed, error:%{public}d", ret);
        return ERROR;
    }
    return ret;
}

int32_t MiscdeviceService::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    CALL_LOG_ENTER;
    if (fd < 0) {
        MISC_HILOGE("Invalid fd");
        return DUMP_PARAM_ERR;
    }
    if (args.empty()) {
        MISC_HILOGE("args cannot be empty");
        dprintf(fd, "args cannot be empty\n");
        DumpHelper->DumpHelp(fd);
        return DUMP_PARAM_ERR;
    }
    std::vector<std::string> argList = { "" };
    std::transform(args.begin(), args.end(), std::back_inserter(argList),
        [](const std::u16string &arg) {
        return Str16ToStr8(arg);
    });
    DumpHelper->ParseCommand(fd, argList);
    return ERR_OK;
}

int32_t MiscdeviceService::PerformVibrationControl(const VibratorIdentifierIPC& identifier,
    int32_t duration, VibrateInfo& info)
{
    std::string curVibrateTime = GetCurrentTime();
    if (StartVibrateThreadControl(identifier, info) != ERR_OK) {
        MISC_HILOGE("%{public}s:vibration is ignored and high priority is vibrating or no vibration found",
            curVibrateTime.c_str());
        return ERROR;
    }
    MISC_HILOGW("Start vibrator, currentTime:%{public}s, package:%{public}s, pid:%{public}d, usage:%{public}d,"
        "duration:%{public}d", curVibrateTime.c_str(), info.packageName.c_str(), info.pid, info.usage, duration);
    return ERR_OK;
}

void MiscdeviceService::ReportCallTimes()
{
    std::time_t now = std::time(nullptr);
    std::tm *nowTm = std::localtime(&now);
    if (nowTm == nullptr) {
        MISC_HILOGE("Get the current time failed!");
        return;
    }
    int32_t hoursToMidnight = HOURS_IN_DAY - 1 - nowTm->tm_hour;
    int32_t minutesToMidnight = MINUTES_IN_HOUR - 1 - nowTm->tm_min;
    int32_t secondsToMidnight = SECONDS_IN_MINUTE - nowTm->tm_sec;
    auto durationToMidnight = std::chrono::hours(hoursToMidnight) + std::chrono::minutes(minutesToMidnight) +
                              std::chrono::seconds(secondsToMidnight);
    std::unique_lock<std::mutex> lock(stopMutex_);
    stopCondition_.wait_for(lock, durationToMidnight, [this] { return stop_.load(); });
    while (!stop_) {
        std::vector<int32_t> vibratorModeStatisticVec = {timeModeCallTimes_.load(), presetModeCallTimes_.load(),
            fileModeCallTimes_.load(), patternModeCallTimes_.load()};
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_STATISTICS", HiSysEvent::EventType::STATISTIC,
            "STATISTICS_TYPE", "VIBRATOR_MODE", "PKG_NAME", "", "STATISTICS_DATA", vibratorModeStatisticVec);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        ReportInvalidVibratorInfo();
        PriorityManager->ReportSwitchStatus();
        MISC_HILOGI("CallTimesReport timeModeCallTimes:%{public}d, presetModeCallTimes:%{public}d, "
                    "fileModeCallTimes:%{public}d, patternModeCallTimes:%{public}d",
            timeModeCallTimes_.load(), presetModeCallTimes_.load(), fileModeCallTimes_.load(),
            patternModeCallTimes_.load());
        timeModeCallTimes_ = 0;
        presetModeCallTimes_ = 0;
        fileModeCallTimes_ = 0;
        patternModeCallTimes_ = 0;
        stopCondition_.wait_for(lock, std::chrono::hours(HOURS_IN_DAY), [this] { return stop_.load(); });
    }
}

void MiscdeviceService::SaveInvalidVibratorInfo(const std::string &pageName, int32_t invalidVibratorId)
{
    std::lock_guard<std::mutex> lock(invalidVibratorInfoMutex_);
    if (invalidVibratorInfoMap_.find(pageName) != invalidVibratorInfoMap_.end()) {
        invalidVibratorInfoMap_[pageName].invalidCallTimes += 1;
        if (invalidVibratorInfoMap_[pageName].maxInvalidVibratorId < invalidVibratorId) {
            invalidVibratorInfoMap_[pageName].maxInvalidVibratorId = invalidVibratorId;
        }
    } else {
        InvalidVibratorInfo invalidVibratorInfo;
        invalidVibratorInfo.maxInvalidVibratorId = invalidVibratorId;
        invalidVibratorInfo.invalidCallTimes = 1;
        invalidVibratorInfoMap_[pageName] = invalidVibratorInfo;
    }
}

void MiscdeviceService::ReportInvalidVibratorInfo()
{
    std::lock_guard<std::mutex> lock(invalidVibratorInfoMutex_);
    if (invalidVibratorInfoMap_.empty()) {
        MISC_HILOGI("invalidVibratorInfoMap_ empty");
        return;
    }
    for (const auto &item : invalidVibratorInfoMap_) {
        std::string pageName = item.first;
        InvalidVibratorInfo invalidVibratorInfo = item.second;
        std::vector<int32_t> vibratorIdInvalidVec = {invalidVibratorInfo.maxInvalidVibratorId,
            invalidVibratorInfo.invalidCallTimes, 0, 0};
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_STATISTICS", HiSysEvent::EventType::STATISTIC,
            "STATISTICS_TYPE", "INVALID_VIBRATOR", "PKG_NAME", pageName, "STATISTICS_DATA", vibratorIdInvalidVec);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
    }
    invalidVibratorInfoMap_.clear();
}

int32_t MiscdeviceService::PlayPattern(const VibratorIdentifierIPC& identifier, const VibratePattern &pattern,
    const CustomHapticInfoIPC& customHapticInfoIPC)
{
    patternModeCallTimes_ += 1;
    int32_t checkResult = PlayPatternCheckAuthAndParam(customHapticInfoIPC.usage, customHapticInfoIPC.parameter);
    if (checkResult != ERR_OK) {
        MISC_HILOGE("CheckAuthAndParam failed, ret:%{public}d", checkResult);
        return checkResult;
    }
    VibratePattern vibratePattern;
    uint32_t sessionId = customHapticInfoIPC.parameter.sessionId;
    vibratePattern.startTime = ((sessionId > 0) ? pattern.startTime : 0);
    vibratePattern.events = pattern.events;
    std::vector<VibratePattern> patterns = {vibratePattern};
    VibratePackage package;
    package.patterns = patterns;
    MergeVibratorParmeters(customHapticInfoIPC.parameter, package);
    package.Dump();
    VibrateInfo info = {
        .mode = VIBRATE_BUTT,
        .packageName = GetPackageName(GetCallingTokenID()),
        .pid = GetCallingPid(),
        .uid = GetCallingUid(),
        .usage = customHapticInfoIPC.usage,
        .systemUsage = customHapticInfoIPC.systemUsage,
        .sessionId = sessionId
    };
    VibratorCapacity capacity;
    if (GetHapticCapacityInfo(identifier, capacity) != ERR_OK) {
        MISC_HILOGE("GetVibratorCapacity failed");
        return ERROR;
    }
    if (capacity.isSupportHdHaptic) {
        int32_t result = PerformVibrationControl(identifier, pattern.patternDuration, info);
        if (result != ERR_OK) {
            MISC_HILOGE("PerformVibrationControl failed");
            return result;
        }
        if (sessionId > 0) {
            return vibratorHdiConnection_.PlayPatternBySessionId(identifier, sessionId, package.patterns.front());
        }
        return vibratorHdiConnection_.PlayPattern(identifier, package.patterns.front());
    } else if (capacity.isSupportPresetMapping) {
        info.mode = VIBRATE_CUSTOM_COMPOSITE_EFFECT;
    } else if (capacity.isSupportTimeDelay) {
        info.mode = VIBRATE_CUSTOM_COMPOSITE_TIME;
    }
    info.package = package;
    return PerformVibrationControl(identifier, pattern.patternDuration, info);
}

int32_t MiscdeviceService::PlayPackageBySessionId(const VibratorIdentifierIPC &identifier,
    const VibratePackage &package, const CustomHapticInfoIPC &customHapticInfoIPC)
{
    CALL_LOG_ENTER;
    int32_t checkResult = PlayPatternCheckAuthAndParam(customHapticInfoIPC.usage, customHapticInfoIPC.parameter);
    if (checkResult != ERR_OK) {
        MISC_HILOGE("CheckAuthAndParam failed, ret:%{public}d", checkResult);
        return checkResult;
    }
    package.Dump();
    VibrateInfo info = {
        .mode = VIBRATE_BUTT,
        .packageName = GetPackageName(GetCallingTokenID()),
        .pid = GetCallingPid(),
        .uid = GetCallingUid(),
        .usage = customHapticInfoIPC.usage,
        .systemUsage = customHapticInfoIPC.systemUsage,
        .sessionId = customHapticInfoIPC.parameter.sessionId
    };
    VibratorCapacity capacity;
    if (GetHapticCapacityInfo(identifier, capacity) != ERR_OK) {
        MISC_HILOGE("GetVibratorCapacity failed");
        return ERROR;
    }
    if (capacity.isSupportHdHaptic) {
        int32_t result = PerformVibrationControl(identifier, package.packageDuration, info);
        if (result != ERR_OK) {
            MISC_HILOGE("PerformVibrationControl failed");
            return result;
        }
        uint32_t sessionId = customHapticInfoIPC.parameter.sessionId;
        return vibratorHdiConnection_.PlayPackageBySessionId(identifier, sessionId, package);
    } else if (capacity.isSupportPresetMapping) {
        info.mode = VIBRATE_CUSTOM_COMPOSITE_EFFECT;
    } else if (capacity.isSupportTimeDelay) {
        info.mode = VIBRATE_CUSTOM_COMPOSITE_TIME;
    }
    info.packageIPC = package;
    return PerformVibrationControl(identifier, package.packageDuration, info);
}

int32_t MiscdeviceService::StopVibrateBySessionId(const VibratorIdentifierIPC &identifier, uint32_t sessionId)
{
    CALL_LOG_ENTER;
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "StopVibrateBySessionId", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("CheckVibratePermission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    std::lock_guard<std::mutex> lock(devicesManageMutex_);
    std::vector<VibratorIdentifierIPC> result = CheckDeviceIdIsValid(identifier);
    size_t ignoreVibrateNum = 0;
    if (result.empty()) {
        MISC_HILOGE("result is empty, no need to stop");
        return ERROR;
    }
    for (const auto& paramIt : result) {
        auto vibratorThread_ = GetVibratorThread(paramIt);
        if (!vibratorHdiConnection_.IsVibratorRunning(paramIt)) {
            MISC_HILOGD("Thread is not running, no need to stop");
            ignoreVibrateNum++;
            continue;
        }
        if (vibratorHdiConnection_.IsVibratorRunning(paramIt)) {
            vibratorHdiConnection_.StopVibrateBySessionId(paramIt, sessionId);
        }
    }
    if (ignoreVibrateNum == result.size()) {
        MISC_HILOGE("No need to stop, ignoreVibrateNum:%{public}zu", ignoreVibrateNum);
        return NO_ERROR;
    }
    std::string packageName = GetPackageName(GetCallingTokenID());
    std::string curVibrateTime = GetCurrentTime();
    MISC_HILOGW("Stop vibrator, currentTime:%{public}s, package:%{public}s, pid:%{public}d, deviceId:%{public}d,"
        "vibratorId:%{public}d, sessionId:%{public}d", curVibrateTime.c_str(), packageName.c_str(), GetCallingPid(),
        identifier.deviceId, identifier.vibratorId, sessionId);
    return NO_ERROR;
}

int32_t MiscdeviceService::PlayPatternCheckAuthAndParam(int32_t usage, const VibrateParameter &parameter)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "PlayPatternStub", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("CheckVibratePermission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    if ((usage >= USAGE_MAX) || (usage < 0) || (!CheckVibratorParmeters(parameter))) {
        MISC_HILOGE("Invalid parameter, usage:%{public}d", usage);
        return PARAMETER_ERROR;
    }
    return ERR_OK;
}

int32_t MiscdeviceService::GetDelayTime(const VibratorIdentifierIPC& identifier, int32_t &delayTime)
{
    std::string packageName = GetPackageName(GetCallingTokenID());
    MISC_HILOGD("GetDelayTime, package:%{public}s", packageName.c_str());
    VibratorCapacity capacity;
    if (GetHapticCapacityInfo(identifier, capacity) != ERR_OK) {
        MISC_HILOGE("GetVibratorCapacity failed");
        return ERROR;
    }
    return vibratorHdiConnection_.GetDelayTime(identifier, capacity.GetVibrateMode(), delayTime);
}

bool MiscdeviceService::CheckVibratorParmeters(const VibrateParameter &parameter)
{
    if ((parameter.intensity < INTENSITY_ADJUST_MIN) || (parameter.intensity > INTENSITY_ADJUST_MAX) ||
        (parameter.frequency < FREQUENCY_ADJUST_MIN) || (parameter.frequency > FREQUENCY_ADJUST_MAX)) {
        MISC_HILOGE("Input invalid, intensity parameter is %{public}d, frequency parameter is %{public}d",
            parameter.intensity, parameter.frequency);
        return false;
    }
    return true;
}

void MiscdeviceService::MergeVibratorParmeters(const VibrateParameter &parameter, VibratePackage &package)
{
    if ((parameter.intensity == INTENSITY_ADJUST_MAX) && (parameter.frequency == 0)) {
        MISC_HILOGD("The adjust parameter is not need to merge");
        return;
    }
    MISC_HILOGI("intensity:%{public}d, frequency:%{public}d", parameter.intensity, parameter.frequency);
    for (VibratePattern &pattern : package.patterns) {
        for (VibrateEvent &event : pattern.events) {
            float intensityScale = static_cast<float>(parameter.intensity) / INTENSITY_ADJUST_MAX;
            if ((event.tag == EVENT_TAG_TRANSIENT) || (event.points.empty())) {
                event.intensity = static_cast<int32_t>(event.intensity * intensityScale);
                event.intensity = std::max(std::min(event.intensity, INTENSITY_MAX), INTENSITY_MIN);
                event.frequency = event.frequency + parameter.frequency;
                event.frequency = std::max(std::min(event.frequency, FREQUENCY_MAX), FREQUENCY_MIN);
            } else {
                for (VibrateCurvePoint &point : event.points) {
                    point.intensity = static_cast<int32_t>(point.intensity * intensityScale);
                    point.intensity = std::max(std::min(point.intensity, INTENSITY_ADJUST_MAX), INTENSITY_ADJUST_MIN);
                    point.frequency = point.frequency + parameter.frequency;
                    point.frequency = std::max(std::min(point.frequency, FREQUENCY_ADJUST_MAX), FREQUENCY_ADJUST_MIN);
                }
            }
        }
    }
}

void MiscdeviceService::SendMsgToClient(const HdfVibratorPlugInfo &info)
{
    MISC_HILOGI("Device:%{public}d state change, state:%{public}d, deviceName:%{public}s", info.deviceId, info.status,
        info.deviceName.c_str());
    std::lock_guard<std::mutex> lockManage(devicesManageMutex_);
    if (info.status == 0) {
        auto it = devicesManageMap_.find(info.deviceId);
        if (it != devicesManageMap_.end()) {
            VibratorIdentifierIPC identifier;
            for (auto &value : it->second.baseInfo) {
                identifier.deviceId = info.deviceId;
                identifier.vibratorId = value.vibratorId;
                StopVibratorService(identifier);
            }
            devicesManageMap_.erase(it);
            MISC_HILOGI("Device %{public}d is offline and removed from the map.", info.deviceId);
        }
    } else {
        std::vector<HdfVibratorInfo> vibratorInfo;
        auto ret = vibratorHdiConnection_.GetVibratorInfo(vibratorInfo);
        if (ret != NO_ERROR || vibratorInfo.empty()) {
            MISC_HILOGE("Device not contain the local vibrator");
        }
        if (InsertVibratorInfo(info.deviceId, info.deviceName, vibratorInfo) != NO_ERROR) {
            MISC_HILOGE("Insert vibrator of device %{public}d fail", info.deviceId);
        }
    }

    std::lock_guard<std::mutex> lock(clientPidMapMutex_);
    MISC_HILOGI("Device:%{public}d state change,state:%{public}d, clientPidMap_.size::%{public}zu",
        info.deviceId, info.status, clientPidMap_.size());
    for (auto it = clientPidMap_.begin(); it != clientPidMap_.end(); ++it) {
        const sptr<IRemoteObject>& key = it->first;

        sptr<IVibratorClient> clientProxy = iface_cast<IVibratorClient>(key);
        if (clientProxy != nullptr) {
            MISC_HILOGI("Device:%{public}d state change,state:%{public}d, ProcessPlugEvent",
                        info.deviceId, info.status);
            clientProxy->ProcessPlugEvent(info.status, info.deviceId, info.vibratorCnt);
        }
    }
}

int32_t MiscdeviceService::TransferClientRemoteObject(const sptr<IRemoteObject> &vibratorServiceClient)
{
    auto clientPid = GetCallingPid();
    if (clientPid < 0) {
        MISC_HILOGE("ClientPid is invalid, clientPid:%{public}d", clientPid);
        return ERROR;
    }
    RegisterClientDeathRecipient(vibratorServiceClient, clientPid);
    return ERR_OK;
}

void MiscdeviceService::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    CALL_LOG_ENTER;
    sptr<IRemoteObject> client = object.promote();
    int32_t clientPid = FindClientPid(client);
    VibrateInfo info;
    std::lock_guard<std::mutex> lock(devicesManageMutex_);
    for (const auto& pair : devicesManageMap_) {
        int deviceId = pair.first;
        const VibratorControlInfo& vibratorControlInfo_ = pair.second.controlInfo;
        MISC_HILOGI("Device ID:%{public}d, , Motor Count:%{public}d", deviceId, vibratorControlInfo_.motorCount);
        for (const auto& motorPair : vibratorControlInfo_.vibratorThreads) {
            int motorId = motorPair.first;
            {
                std::lock_guard<std::mutex> lock(vibratorThreadMutex_);
                auto vibratorThread_ = motorPair.second;
                if (vibratorThread_ == nullptr) {
                    MISC_HILOGE("MotorId:%{public}d, Vibrate thread no found in devicesManageMap_", motorId);
                    vibratorThread_ = std::make_shared<VibratorThread>();
                }
                info = vibratorThread_->GetCurrentVibrateInfo();
            }
            int32_t vibratePid = info.pid;
            MISC_HILOGI("ClientPid:%{public}d, VibratePid:%{public}d", clientPid, vibratePid);
            if ((clientPid != INVALID_PID) && (clientPid == vibratePid)) {
                VibratorIdentifierIPC identifier;
                identifier.deviceId = deviceId;
                identifier.vibratorId = motorId;
                StopVibratorService(identifier);
            }
        }
    }
    UnregisterClientDeathRecipient(client);
}

void  MiscdeviceService::RegisterClientDeathRecipient(sptr<IRemoteObject> vibratorServiceClient, int32_t pid)
{
    if (vibratorServiceClient == nullptr) {
        MISC_HILOGE("VibratorServiceClient is nullptr");
        return;
    }
    std::lock_guard<std::mutex> lock(clientDeathObserverMutex_);
    if (clientDeathObserver_ == nullptr) {
        clientDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<MiscdeviceService *>(this));
        if (clientDeathObserver_ == nullptr) {
            MISC_HILOGE("ClientDeathObserver_ is nullptr");
            return;
        }
    }
    vibratorServiceClient->AddDeathRecipient(clientDeathObserver_);
    SaveClientPid(vibratorServiceClient, pid);
}

void MiscdeviceService::UnregisterClientDeathRecipient(sptr<IRemoteObject> vibratorServiceClient)
{
    if (vibratorServiceClient == nullptr) {
        MISC_HILOGE("vibratorServiceClient is nullptr");
        return;
    }
    int32_t clientPid = FindClientPid(vibratorServiceClient);
    if (clientPid == INVALID_PID) {
        MISC_HILOGE("Pid is invalid");
        return;
    }
    std::lock_guard<std::mutex> lock(clientDeathObserverMutex_);
    vibratorServiceClient->RemoveDeathRecipient(clientDeathObserver_);
    DestroyClientPid(vibratorServiceClient);
}

void MiscdeviceService::SaveClientPid(const sptr<IRemoteObject> &vibratorServiceClient, int32_t pid)
{
    if (vibratorServiceClient == nullptr) {
        MISC_HILOGE("VibratorServiceClient is nullptr");
        return;
    }
    std::lock_guard<std::mutex> lock(clientPidMapMutex_);
    if (clientPidMap_.size() >= MAX_SUPPORT_CLIENT_NUM) {
        MISC_HILOGE("The maximum number of supported clients has been exceeded");
        return;
    }
    clientPidMap_.insert(std::make_pair(vibratorServiceClient, pid));
}

int32_t MiscdeviceService::FindClientPid(const sptr<IRemoteObject> &vibratorServiceClient)
{
    if (vibratorServiceClient == nullptr) {
        MISC_HILOGE("VibratorServiceClient is nullptr");
        return INVALID_PID;
    }
    std::lock_guard<std::mutex> lock(clientPidMapMutex_);
    auto it = clientPidMap_.find(vibratorServiceClient);
    if (it == clientPidMap_.end()) {
        MISC_HILOGE("Cannot find client pid");
        return INVALID_PID;
    }
    return it->second;
}

void MiscdeviceService::DestroyClientPid(const sptr<IRemoteObject> &vibratorServiceClient)
{
    if (vibratorServiceClient == nullptr) {
        MISC_HILOGD("VibratorServiceClient is nullptr");
        return;
    }
    std::lock_guard<std::mutex> lock(clientPidMapMutex_);
    auto it = clientPidMap_.find(vibratorServiceClient);
    if (it == clientPidMap_.end()) {
        MISC_HILOGE("Cannot find client pid");
        return;
    }
    clientPidMap_.erase(it);
}

int32_t MiscdeviceService::PlayPrimitiveEffect(const VibratorIdentifierIPC& identifier,
    const std::string &effect, const PrimitiveEffectIPC& primitiveEffectIPC)
{
    presetModeCallTimes_ += 1;
    int32_t checkResult = PlayPrimitiveEffectCheckAuthAndParam(primitiveEffectIPC.intensity, primitiveEffectIPC.usage);
    if (checkResult != ERR_OK) {
        MISC_HILOGE("CheckAuthAndParam failed, ret:%{public}d", checkResult);
        return checkResult;
    }
#ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
    std::optional<HdfEffectInfo> effectInfo = vibratorHdiConnection_.GetEffectInfo(identifier, effect);
    if (!effectInfo) {
        MISC_HILOGE("GetEffectInfo fail");
        return ERROR;
    }
    if (!(effectInfo->isSupportEffect)) {
        MISC_HILOGE("Effect not supported");
        return DEVICE_OPERATION_FAILED;
    }
#endif // HDF_DRIVERS_INTERFACE_VIBRATOR
    VibrateInfo info = {
        .mode = VIBRATE_PRESET,
        .packageName = GetPackageName(GetCallingTokenID()),
        .pid = GetCallingPid(),
        .uid = GetCallingUid(),
        .usage = primitiveEffectIPC.usage,
        .systemUsage = primitiveEffectIPC.systemUsage,
#ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
        .duration = effectInfo->duration,
#endif // HDF_DRIVERS_INTERFACE_VIBRATOR
        .effect = effect,
        .count = primitiveEffectIPC.count,
        .intensity = primitiveEffectIPC.intensity
    };
    std::string curVibrateTime = GetCurrentTime();
    if (StartVibrateThreadControl(identifier, info) != ERR_OK) {
        MISC_HILOGE("%{public}s:vibration is ignored and high priority is vibrating or no vibration found",
            curVibrateTime.c_str());
        return ERROR;
    }
    MISC_HILOGW("Start vibrator, currentTime:%{public}s, package:%{public}s, pid:%{public}d, usage:%{public}d,"
        "deviceId:%{public}d, vibratorId:%{public}d, duration:%{public}d, effect:%{public}s, intensity:%{public}d",
        curVibrateTime.c_str(), info.packageName.c_str(), info.pid, info.usage, identifier.deviceId,
        identifier.vibratorId, info.duration, info.effect.c_str(), info.intensity);
    return NO_ERROR;
}

int32_t MiscdeviceService::PlayPrimitiveEffectCheckAuthAndParam(int32_t intensity, int32_t usage)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "PlayPrimitiveEffectStub", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        MISC_HILOGE("CheckVibratePermission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    if ((intensity <= INTENSITY_MIN) || (intensity > INTENSITY_MAX) || (usage >= USAGE_MAX) || (usage < 0)) {
        MISC_HILOGE("Invalid parameter");
        return PARAMETER_ERROR;
    }
    return ERR_OK;
}

int32_t MiscdeviceService::GetLocalDeviceId(int32_t &deviceId)
{
    for (const auto& device : devicesManageMap_) {
        for (const auto& info : device.second.baseInfo) {
            if (info.isLocalVibrator) {
                deviceId = info.deviceId;
                return NO_ERROR;
            }
        }
    }

    return ERROR;
}

int32_t MiscdeviceService::GetOneVibrator(const VibratorIdentifierIPC& actIdentifier,
    std::vector<VibratorInfoIPC>& vibratorInfoIPC)
{
    if (devicesManageMap_.empty()) {
        MISC_HILOGE("No vibrator device online");
        return ERROR;
    }

    auto it = devicesManageMap_.find(actIdentifier.deviceId);
    if (it == devicesManageMap_.end()) {
        MISC_HILOGE("Device manager map has no vibrator info");
        return ERROR;
    }
    for (auto &value : it->second.baseInfo) {
        if (actIdentifier.vibratorId == value.vibratorId) {
            vibratorInfoIPC.emplace_back(value);
            break;
        }
    }
    return NO_ERROR;
}

int32_t MiscdeviceService::GetVibratorList(const VibratorIdentifierIPC& identifier,
    std::vector<VibratorInfoIPC>& vibratorInfoIPC)
{
    CALL_LOG_ENTER;
    identifier.Dump();
    GetOnlineVibratorInfo();
    std::lock_guard<std::mutex> lockManage(devicesManageMutex_);
    if ((identifier.deviceId == -1) && (identifier.vibratorId == -1)) {
        for (auto &value : devicesManageMap_) {
            vibratorInfoIPC.insert(vibratorInfoIPC.end(), value.second.baseInfo.begin(),
                value.second.baseInfo.end());
        }
    } else if ((identifier.deviceId == -1) && (identifier.vibratorId != -1)) {
        VibratorIdentifierIPC actIdentifier;
        actIdentifier.vibratorId = identifier.vibratorId;
        int32_t ret = GetLocalDeviceId(actIdentifier.deviceId);
        if (ret == NO_ERROR) {
            ret = GetOneVibrator(actIdentifier, vibratorInfoIPC);
            if (ret != NO_ERROR) {
                MISC_HILOGI("Local deviceId %{public}d has no vibratorId %{public}d info",
                    actIdentifier.deviceId, actIdentifier.vibratorId);
            }
        }
    } else if ((identifier.deviceId != -1) && (identifier.vibratorId == -1)) {
        auto it = devicesManageMap_.find(identifier.deviceId);
        if (it == devicesManageMap_.end()) {
            MISC_HILOGD("Device manager map has no vibrator info");
            return NO_ERROR;
        }
        vibratorInfoIPC = it->second.baseInfo;
    } else { // ((identifier.deviceId != -1) && (identifier.vibratorId != -1))
        if (GetOneVibrator(identifier, vibratorInfoIPC) != NO_ERROR) {
            MISC_HILOGI("DeviceId %{public}d has no vibratorId %{public}d info",
                identifier.deviceId, identifier.vibratorId);
        }
    }
    return NO_ERROR;
}

int32_t MiscdeviceService::GetEffectInfo(const VibratorIdentifierIPC& identifier, const std::string& effectType,
    EffectInfoIPC& effectInfoIPC)
{
    CALL_LOG_ENTER;
    identifier.Dump();
    if (devicesManageMap_.empty()) {
        MISC_HILOGI("No vibrator device online");
        return NO_ERROR;
    }
    VibratorIdentifierIPC localIdentifier;
    if (identifier.deviceId == -1) {
        for (const auto& pair : devicesManageMap_) {
            for (const auto& info : pair.second.baseInfo) {
                info.isLocalVibrator ? (localIdentifier.deviceId = info.deviceId) : 0;
            }
        }
    }
    HdfEffectInfo hdfEffectInfo;
    int32_t ret = vibratorHdiConnection_.GetEffectInfo((identifier.deviceId == -1) ? localIdentifier : identifier,
        effectType, hdfEffectInfo);
    if (ret != NO_ERROR) {
        MISC_HILOGE("HDI::GetEffectInfo return error");
        return ERROR;
    }
    effectInfoIPC.duration = hdfEffectInfo.duration;
    effectInfoIPC.isSupportEffect = hdfEffectInfo.isSupportEffect;
    effectInfoIPC.Dump();
    return NO_ERROR;
}

int32_t MiscdeviceService::SubscribeVibratorPlugInfo(const sptr<IRemoteObject> &vibratorServiceClient)
{
    auto clientPid = GetCallingPid();
    if (clientPid < 0) {
        MISC_HILOGE("ClientPid is invalid, clientPid:%{public}d", clientPid);
        return ERROR;
    }
    if (vibratorServiceClient == nullptr) {
        MISC_HILOGD("VibratorServiceClient is nullptr");
        return ERROR;
    }
    return ERR_OK;
}

int32_t MiscdeviceService::GetVibratorCapacity(const VibratorIdentifierIPC& identifier,
    VibratorCapacity &vibratorCapacity)
{
    CALL_LOG_ENTER;
    VibratorCapacity capacity;
    if (GetHapticCapacityInfo(identifier, capacity) != ERR_OK) {
        MISC_HILOGE("GetVibratorCapacity failed");
        return ERROR;
    }
    vibratorCapacity = capacity;
    return ERR_OK;
}

// 手表短振快速下发不启动线程
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_PRESET_INFO
int32_t MiscdeviceService::FastVibratorEffect(const VibrateInfo &info, const VibratorIdentifierIPC& identifier)
{
    int32_t ret = VibratorDevice.StartByIntensity(identifier, info.effect, info.intensity);
    if (ret != SUCCESS) {
        MISC_HILOGE("Vibrate effect %{public}s failed.", info.effect.c_str());
    }
    return NO_ERROR;
}
#endif // OHOS_BUILD_ENABLE_VIBRATOR_PRESET_INFO

std::shared_ptr<VibratorThread> MiscdeviceService::GetVibratorThread(const VibratorIdentifierIPC& identifier)
{
    CALL_LOG_ENTER;
    auto deviceIt = devicesManageMap_.find(identifier.deviceId);
    if (deviceIt != devicesManageMap_.end()) {
        auto thread = deviceIt->second.controlInfo.GetVibratorThread(identifier.vibratorId);
        if (thread) {
            MISC_HILOGI("Success: Vibrate thread found.");
            return thread;
        } else {
            MISC_HILOGE("Failed: Vibrate thread no found.");
            return nullptr;
        }
    }
    return nullptr;
}

int32_t MiscdeviceService::GetHapticCapacityInfo(const VibratorIdentifierIPC& identifier,
    VibratorCapacity& capacityInfo)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> lock(devicesManageMutex_);
    if (identifier.deviceId != -1) {
        auto deviceIt = devicesManageMap_.find(identifier.deviceId);
        if (deviceIt != devicesManageMap_.end()) {
            capacityInfo = deviceIt->second.capacityInfo;
            return ERR_OK;
        }
        MISC_HILOGW("No vibrator capacity information found for device ID: %{public}d", identifier.deviceId);
        return ERR_OK;
    }
    for (const auto& pair : devicesManageMap_) {
        for (const auto& info : pair.second.baseInfo) {
            if (info.isLocalVibrator) {
                capacityInfo = pair.second.capacityInfo;
                return ERR_OK;
            }
        }
    }
    MISC_HILOGW("No local vibrator capacity information found for device ID: %{public}d", identifier.deviceId);
    return ERR_OK;
}

int32_t MiscdeviceService::GetAllWaveInfo(const VibratorIdentifierIPC& identifier,
    std::vector<HdfWaveInformation>& waveInfo)
{
    CALL_LOG_ENTER;
    if (identifier.deviceId != -1) {
        auto deviceIt = devicesManageMap_.find(identifier.deviceId);
        if (deviceIt != devicesManageMap_.end()) {
            waveInfo = deviceIt->second.waveInfo;
            return ERR_OK;
        }
        MISC_HILOGE("Device ID %{public}d not found", identifier.deviceId);
        return ERR_OK;
    }
    for (const auto& [deviceId, deviceInfo] : devicesManageMap_) {
        for (const auto& baseInfo : deviceInfo.baseInfo) {
            if (baseInfo.isLocalVibrator) {
                waveInfo = deviceInfo.waveInfo;
                return ERR_OK;
            }
        }
    }
    MISC_HILOGW("No vibrator information found for device ID: %{public}d", identifier.deviceId);
    return ERR_OK;
}

int32_t MiscdeviceService::GetHapticStartUpTime(const VibratorIdentifierIPC& identifier, int32_t mode,
    int32_t &startUpTime)
{
    CALL_LOG_ENTER;
    auto ret = vibratorHdiConnection_.GetDelayTime(identifier, mode, startUpTime);
    if (ret != NO_ERROR) {
        MISC_HILOGE("HDI::GetHapticStartUpTime return error");
        return ERROR;
    }
    return NO_ERROR;
}

void MiscdeviceService::ConvertToServerInfos(const std::vector<HdfVibratorInfo> &baseVibratorInfo,
    const VibratorCapacity &vibratorCapacity, const std::vector<HdfWaveInformation> &waveInfomation,
    const HdfVibratorPlugInfo &info, VibratorAllInfos& vibratorAllInfos)
{
    CALL_LOG_ENTER;
    VibratorInfoIPC vibratorInfo;
    for (auto infos : baseVibratorInfo) {
        vibratorInfo.deviceId = infos.deviceId;
        vibratorInfo.vibratorId = infos.vibratorId;
        vibratorInfo.deviceName = info.deviceName;
        vibratorInfo.isSupportHdHaptic = vibratorCapacity.isSupportHdHaptic;
        vibratorInfo.isLocalVibrator = infos.isLocal;
        vibratorAllInfos.baseInfo.emplace_back(vibratorInfo);
        MISC_HILOGI("HDI::HdfVibratorInfo deviceName:%{public}s, deviceId:%{public}d, vibratorId:%{public}d,"
            " isLocalVibrator:%{public}d", info.deviceName.c_str(), vibratorInfo.deviceId, vibratorInfo.vibratorId,
            vibratorInfo.isLocalVibrator);
    }
    vibratorAllInfos.capacityInfo = vibratorCapacity;
    vibratorAllInfos.waveInfo = waveInfomation;
}

void MiscdeviceService::GetOnlineVibratorInfo()
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> lockManage(devicesManageMutex_);
    if (!devicesManageMap_.empty()) {
        MISC_HILOGD("devicesManageMap_ not empty");
        return;
    }
    std::vector<HdfVibratorInfo> vibratorInfo;
    auto ret = vibratorHdiConnection_.GetVibratorInfo(vibratorInfo);
    if (ret != NO_ERROR || vibratorInfo.empty()) {
        MISC_HILOGW("Device does not contain any vibrators");
        return;
    }

    const std::string deviceName = "";
    for (auto &info : vibratorInfo) {
        const auto it = devicesManageMap_.find(info.deviceId);
        if (it != devicesManageMap_.end()) {
            continue;
        }
        if (InsertVibratorInfo(info.deviceId, deviceName, vibratorInfo) != NO_ERROR) {
            MISC_HILOGW("Insert vibrator of device %{public}d fail", info.deviceId);
        }
    }
}

int32_t MiscdeviceService::InsertVibratorInfo(int deviceId, const std::string &deviceName,
    const std::vector<HdfVibratorInfo> &vibratorInfo)
{
    CALL_LOG_ENTER;
    std::vector<HdfVibratorInfo> infos;
    std::vector<int> vibratorIdList;
    for (auto &info : vibratorInfo) {
        const auto it = devicesManageMap_.find(info.deviceId);
        if (it != devicesManageMap_.end()) {
            MISC_HILOGW("The deviceId already exists in devicesManageMap_, deviceId: %{public}d", info.deviceId);
            continue;
        }
        if (info.deviceId == deviceId) {
            infos.emplace_back(info);
            vibratorIdList.push_back(info.vibratorId);
        }
    }
    if (infos.empty()) {
        MISC_HILOGW("Device %{public}d does not contain any vibrators", deviceId);
        return NO_ERROR;
    }

    VibratorIdentifierIPC param;
    param.deviceId = infos[0].deviceId;
    param.vibratorId = infos[0].vibratorId;
    VibratorCapacity capacity;
    int32_t ret = vibratorHdiConnection_.GetVibratorCapacity(param, capacity);
    if (ret != NO_ERROR) {
        MISC_HILOGW("Get capacity fail from HDI, then use the default capacity, deviceId: %{public}d", param.deviceId);
    }
    std::vector<HdfWaveInformation> waveInfo;
    ret = vibratorHdiConnection_.GetAllWaveInfo(param, waveInfo);
    if (ret != NO_ERROR) {
        MISC_HILOGW("Get waveInfo fail from HDI, deviceId: %{public}d", param.deviceId);
    }

    HdfVibratorPlugInfo mockInfo;
    mockInfo.deviceName = deviceName;
    VibratorAllInfos localVibratorInfo(vibratorIdList);
    (void)ConvertToServerInfos(infos, capacity, waveInfo, mockInfo, localVibratorInfo);
    devicesManageMap_.insert(std::make_pair(param.deviceId, localVibratorInfo));
    return NO_ERROR;
}

int32_t MiscdeviceService::StartVibrateThreadControl(const VibratorIdentifierIPC& identifier, VibrateInfo& info)
{
    std::lock_guard<std::mutex> lockManage(devicesManageMutex_);
    std::string curVibrateTime = GetCurrentTime();
    std::vector<VibratorIdentifierIPC> result = CheckDeviceIdIsValid(identifier);
    if (result.empty()) {
        MISC_HILOGE("No vibration found");
        return ERROR;
    }
    {
        std::lock_guard<std::mutex> guard(pidMutex_);
        if (std::find(disablePids_.begin(), disablePids_.end(), info.pid) != disablePids_.end()) {
            MISC_HILOGE("Pid :%{public}d is disabled, reject vibration", info.pid);
            return ERROR;
        }
    }
    size_t ignoreVibrateNum = 0;

    const std::vector<std::string> specialModes = {
        VIBRATE_CUSTOM_HD, VIBRATE_CUSTOM_COMPOSITE_EFFECT,
        VIBRATE_CUSTOM_COMPOSITE_TIME, VIBRATE_BUTT
    };

    std::unordered_set<int32_t> uniqueIndices;
    if (std::find(specialModes.begin(), specialModes.end(), info.mode) != specialModes.end()) {
        for (const auto& pattern : info.package.patterns) {
            for (const auto& event : pattern.events) {
                uniqueIndices.insert(event.index);
            }
        }
        for (const auto& index : uniqueIndices) {
            MISC_HILOGD("Info mode:%{public}s, vibratorIndex:%{public}d", info.mode.c_str(), index);
        }
    }
    std::lock_guard<std::mutex> lock(vibratorThreadMutex_);
    for (const auto& paramIt : result) {
        bool shouldProcess = uniqueIndices.empty() ||
        uniqueIndices.find(0) != uniqueIndices.end() ||
        uniqueIndices.find(paramIt.position) != uniqueIndices.end();

        if (paramIt.isLocalVibrator && ShouldIgnoreVibrate(info, paramIt)) {
            if (shouldProcess) {
                ignoreVibrateNum++;
                continue;
            }
        }
        if (shouldProcess) {
            StartVibrateThread(info, paramIt);
        }
    }

    return (ignoreVibrateNum == result.size()) ? ERROR : ERR_OK;
}

bool MiscdeviceService::IsVibratorIdValid(const std::vector<VibratorInfoIPC> baseInfo, int32_t target)
{
    for (const auto& item : baseInfo) {
        if (item.vibratorId == target || target == -1) {
            return true;
        }
    }
    std::string packageName = GetPackageName(GetCallingTokenID());
    invalidVibratorIdCount_++;
    if (invalidVibratorIdCount_ == LOG_COUNT_FIVE) {
        invalidVibratorIdCount_ = 0;
        MISC_HILOGE("VibratorId is not valid. package:%{public}s vibratorid:%{public}d", packageName.c_str(), target);
    }
    SaveInvalidVibratorInfo(packageName, target);
    return false;
}

std::vector<VibratorIdentifierIPC> MiscdeviceService::CheckDeviceIdIsValid(const VibratorIdentifierIPC& identifier)
{
    CALL_LOG_ENTER;
    std::vector<VibratorIdentifierIPC> result;

    auto addToResult = [&](const auto& info) {
        VibratorIdentifierIPC newIdentifier;
        newIdentifier.deviceId = info.deviceId;
        newIdentifier.vibratorId = info.vibratorId;
        newIdentifier.position = info.position;
        newIdentifier.isLocalVibrator = info.isLocalVibrator;
        result.push_back(newIdentifier);
        MISC_HILOGD("Push result in list,:%{public}d,:%{public}d", newIdentifier.deviceId, newIdentifier.vibratorId);
    };

    auto processDevice = [&](const auto& device) {
        for (const auto& info : device.second.baseInfo) {
            bool shouldAdd = false;
            if (identifier.deviceId == -1 && identifier.vibratorId != -1) {
                shouldAdd = info.isLocalVibrator && info.vibratorId == identifier.vibratorId;
            } else if (identifier.deviceId != -1 && identifier.vibratorId != -1) {
                shouldAdd = info.vibratorId == identifier.vibratorId;
            } else if (identifier.deviceId == -1 && identifier.vibratorId == -1) {
                shouldAdd = info.isLocalVibrator;
            } else { // (identifier.deviceId != -1 && identifier.vibratorId == -1)
                shouldAdd = true;
            }

            if (shouldAdd) {
                addToResult(info);
            }
        }
    };

    if (identifier.deviceId != -1) {
        auto deviceIt = devicesManageMap_.find(identifier.deviceId);
        if (deviceIt != devicesManageMap_.end()) {
            processDevice(*deviceIt);
        }
        return result;
    }

    for (const auto& pair : devicesManageMap_) {
        if (!IsVibratorIdValid(pair.second.baseInfo, identifier.vibratorId)) {
            for (const auto& info : pair.second.baseInfo) {
                addToResult(info);
            }
        } else {
            processDevice(pair);
        }
    }

    return result;
}

int32_t MiscdeviceService::DisableVibratorByPid(int32_t pid)
{
    CALL_LOG_ENTER;
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "PlayPrimitiveEffectStub", "ERROR_CODE", ret);
        MISC_HILOGE("CheckVibratePermission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    bool result = VibrationPriorityManager::IsSystemServiceCalling();
    if (!result) {
        MISC_HILOGE("Non-system service call");
        return PERMISSION_DENIED;
    }
    MISC_HILOGD("Disable pid: %{public}d", pid);
    std::lock_guard<std::mutex> guard(pidMutex_);
    auto it = std::find(disablePids_.begin(), disablePids_.end(), pid);
    if (it == disablePids_.end()) {
        disablePids_.push_back(pid);
        MISC_HILOGD("Pid %{public}d added to disabled list", pid);
    }
    return ERR_OK;
}

int32_t MiscdeviceService::EnableVibratorByPid(int32_t pid)
{
    CALL_LOG_ENTER;
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "PlayPrimitiveEffectStub", "ERROR_CODE", ret);
        MISC_HILOGE("CheckVibratePermission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    bool result = VibrationPriorityManager::IsSystemServiceCalling();
    if (!result) {
        MISC_HILOGE("Non-system service call");
        return PERMISSION_DENIED;
    }
    MISC_HILOGD("Disable pid: %{public}d", pid);
    std::lock_guard<std::mutex> guard(pidMutex_);
    auto it = std::find(disablePids_.begin(), disablePids_.end(), pid);
    if (it != disablePids_.end()) {
        disablePids_.erase(it);
        MISC_HILOGD("Pid %{public}d removed from disabled list", pid);
    }
    return ERR_OK;
}
}  // namespace Sensors
}  // namespace OHOS
