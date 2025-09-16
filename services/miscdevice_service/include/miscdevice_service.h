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

#ifndef MISCDEVICE_SERVICE_H
#define MISCDEVICE_SERVICE_H

#include "accesstoken_kit.h"
#include "common_event_manager.h"
#include "system_ability.h"

#include "light_hdi_connection.h"
#include "miscdevice_common.h"
#include "miscdevice_common_event_subscriber.h"
#include "miscdevice_delayed_sp_singleton.h"
#include "miscdevice_dump.h"
#include "miscdevice_service_stub.h"
#include "vibrator_thread.h"

namespace OHOS {
namespace Sensors {
using namespace Security::AccessToken;
enum class MiscdeviceServiceState {
    STATE_STOPPED,
    STATE_RUNNING,
};

struct VibratorControlInfo {
    int motorCount;
    std::unordered_map<int, std::shared_ptr<VibratorThread>> vibratorThreads;

    VibratorControlInfo(const std::vector<int>& vibratorIds) : motorCount(static_cast<int>(vibratorIds.size()))
    {
        for (int motorId : vibratorIds) {
            auto it = vibratorThreads.find(motorId);
            if (it == vibratorThreads.end()) {
                vibratorThreads[motorId] = std::make_shared<VibratorThread>();
            }
        }
    }

    std::shared_ptr<VibratorThread> GetVibratorThread(int motorId) const
    {
        auto it = vibratorThreads.find(motorId);
        if (it != vibratorThreads.end()) {
            return it->second;
        }
        return nullptr;
    }
};

struct VibratorAllInfos {
    std::vector<VibratorInfoIPC> baseInfo;
    VibratorControlInfo controlInfo;
    VibratorCapacity capacityInfo;
    std::vector<HdfWaveInformation> waveInfo;
    VibratorAllInfos(const std::vector<int>& vibratorIds) : controlInfo(vibratorIds) {}
};

struct InvalidVibratorInfo {
    int32_t maxInvalidVibratorId;
    int32_t invalidCallTimes;
};

class MiscdeviceService : public SystemAbility, public MiscdeviceServiceStub {
    DECLARE_SYSTEM_ABILITY(MiscdeviceService)
    MISCDEVICE_DECLARE_DELAYED_SP_SINGLETON(MiscdeviceService);

public:
    void OnDump() override;
    void OnStart() override;
    void OnStop() override;
    void OnStartFuzz();
    bool IsValid(int32_t lightId);
    bool IsLightAnimationValid(const LightAnimationIPC &animation);
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args) override;
    void ProcessDeathObserver(const wptr<IRemoteObject> &object);
    virtual int32_t Vibrate(const VibratorIdentifierIPC& identifier, int32_t timeOut,
        int32_t usage, bool systemUsage) override;
    virtual int32_t PlayVibratorEffect(const VibratorIdentifierIPC& identifier, const std::string &effect,
        int32_t loopCount, int32_t usage, bool systemUsage) override;
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    virtual int32_t PlayVibratorCustom(const VibratorIdentifierIPC& identifier, int32_t fd, int64_t offset,
        int64_t length, const CustomHapticInfoIPC& customHapticInfoIPC) override;
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    virtual int32_t StopVibrator(const VibratorIdentifierIPC& identifier) override;
    virtual int32_t StopVibratorByMode(const VibratorIdentifierIPC& identifier, const std::string &mode) override;
    virtual int32_t IsSupportEffect(const VibratorIdentifierIPC& identifier, const std::string &effect,
        bool &state) override;
    virtual int32_t GetLightList(std::vector<LightInfoIPC> &lightInfoIpcList) override;
    virtual int32_t TurnOn(int32_t lightId, int32_t singleColor, const LightAnimationIPC &animation) override;
    virtual int32_t TurnOff(int32_t lightId) override;
    virtual int32_t PlayPattern(const VibratorIdentifierIPC& identifier, const VibratePattern &pattern,
        const CustomHapticInfoIPC& customHapticInfoIPC) override;

    virtual int32_t PlayPackageBySessionId(const VibratorIdentifierIPC &identifier, const VibratePackageIPC &package,
        const CustomHapticInfoIPC &customHapticInfoIPC) override;
    virtual int32_t StopVibrateBySessionId(const VibratorIdentifierIPC &identifier, uint32_t sessionId) override;
    virtual int32_t GetDelayTime(const VibratorIdentifierIPC& identifier, int32_t &delayTime) override;
    virtual int32_t TransferClientRemoteObject(const sptr<IRemoteObject> &vibratorServiceClient) override;
    virtual int32_t PlayPrimitiveEffect(const VibratorIdentifierIPC& identifier, const std::string &effect,
        const PrimitiveEffectIPC& primitiveEffectIPC) override;
    virtual int32_t GetVibratorCapacity(const VibratorIdentifierIPC& identifier, VibratorCapacity &capacity) override;
    virtual int32_t GetVibratorList(const VibratorIdentifierIPC& identifier,
        std::vector<VibratorInfoIPC>& vibratorInfoIPC) override;
    virtual int32_t GetEffectInfo(const VibratorIdentifierIPC& identifier, const std::string& effectType,
        EffectInfoIPC& effectInfoIPC) override;
    virtual int32_t SubscribeVibratorPlugInfo(const sptr<IRemoteObject> &vibratorServiceClient) override;

private:
    DISALLOW_COPY_AND_MOVE(MiscdeviceService);
    bool InitInterface();
    bool InitLightInterface();
    std::string GetPackageName(AccessTokenID tokenId);
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_PRESET_INFO
    int32_t FastVibratorEffect(const VibrateInfo &info, const VibratorIdentifierIPC& identifier);
#endif // OHOS_BUILD_ENABLE_VIBRATOR_PRESET_INFO
    void StartVibrateThread(VibrateInfo info, const VibratorIdentifierIPC& identifier);
    int32_t StopVibratorService(const VibratorIdentifierIPC& identifier);
    void SendMsgToClient(const HdfVibratorPlugInfo &info);
    int32_t RegisterVibratorPlugCb();
    std::shared_ptr<VibratorThread> GetVibratorThread(const VibratorIdentifierIPC& identifier);
    void StopVibrateThread(std::shared_ptr<VibratorThread> vibratorThread);
    bool ShouldIgnoreVibrate(const VibrateInfo &info, const VibratorIdentifierIPC& identifier);
    std::string GetCurrentTime();
    void MergeVibratorParmeters(const VibrateParameter &parameter, VibratePackage &package);
    bool CheckVibratorParmeters(const VibrateParameter &parameter);
    void RegisterClientDeathRecipient(sptr<IRemoteObject> vibratorServiceClient, int32_t pid);
    void UnregisterClientDeathRecipient(sptr<IRemoteObject> vibratorServiceClient);
    void SaveClientPid(const sptr<IRemoteObject> &vibratorServiceClient, int32_t pid);
    int32_t FindClientPid(const sptr<IRemoteObject> &vibratorServiceClient);
    void DestroyClientPid(const sptr<IRemoteObject> &vibratorServiceClient);
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    int32_t SubscribeCommonEvent(const std::string &eventName, EventReceiver receiver);
    void OnReceiveEvent(const EventFwk::CommonEventData &data);
#ifdef OHOS_BUILD_ENABLE_DO_NOT_DISTURB
    void OnReceiveUserSwitchEvent(const EventFwk::CommonEventData &data);
#endif // OHOS_BUILD_ENABLE_DO_NOT_DISTURB
    int32_t CheckAuthAndParam(int32_t usage, const VibrateParameter &parameter,
        const VibratorIdentifierIPC& identifier);
    int32_t PlayPatternCheckAuthAndParam(int32_t usage, const VibrateParameter &parameter);
    int32_t PlayPrimitiveEffectCheckAuthAndParam(int32_t intensity, int32_t usage);
    int32_t PlayVibratorEffectCheckAuthAndParam(int32_t count, int32_t usage);
    int32_t GetHapticCapacityInfo(const VibratorIdentifierIPC& identifier, VibratorCapacity& capacityInfo);
    int32_t GetAllWaveInfo(const VibratorIdentifierIPC& identifier, std::vector<HdfWaveInformation>& waveInfo);
    int32_t GetHapticStartUpTime(const VibratorIdentifierIPC& identifier, int32_t mode, int32_t &startUpTime);
    void GetOnlineVibratorInfo();
    std::vector<VibratorIdentifierIPC> CheckDeviceIdIsValid(const VibratorIdentifierIPC& identifier);
    int32_t StartVibrateThreadControl(const VibratorIdentifierIPC& identifier, VibrateInfo& info);
    int32_t InsertVibratorInfo(int deviceId, const std::string &deviceName,
        const std::vector<HdfVibratorInfo> &vibratorInfo);
    int32_t GetLocalDeviceId(int32_t &deviceId);
    int32_t GetOneVibrator(const VibratorIdentifierIPC& actIdentifier,
        std::vector<VibratorInfoIPC>& vibratorInfoIPC);
    void ConvertToServerInfos(const std::vector<HdfVibratorInfo> &baseVibratorInfo,
        const VibratorCapacity &vibratorCapacity, const std::vector<HdfWaveInformation> &waveInfomation,
        const HdfVibratorPlugInfo &info, VibratorAllInfos &vibratorAllInfos);
    int32_t PerformVibrationControl(const VibratorIdentifierIPC& identifier, int32_t duration, VibrateInfo& info);
    bool IsVibratorIdValid(const std::vector<VibratorInfoIPC> baseInfo, int32_t target);
    void ReportCallTimes();
    void SaveInvalidVibratorInfo(const std::string &pageName, int32_t invalidVibratorId);
    void ReportInvalidVibratorInfo();
    static std::atomic_int32_t timeModeCallTimes_;
    static std::atomic_int32_t presetModeCallTimes_;
    static std::atomic_int32_t fileModeCallTimes_;
    static std::atomic_int32_t patternModeCallTimes_;
    static std::atomic_bool stop_;
    static std::unordered_map<std::string, InvalidVibratorInfo> invalidVibratorInfoMap_;
    std::thread reportCallTimesThread_;
    std::mutex invalidVibratorInfoMutex_;
    std::mutex stopMutex_;
    std::condition_variable stopCondition_;
    std::mutex isVibrationPriorityReadyMutex_;
    static bool isVibrationPriorityReady_;
    VibratorHdiConnection &vibratorHdiConnection_ = VibratorHdiConnection::GetInstance();
    LightHdiConnection &lightHdiConnection_ = LightHdiConnection::GetInstance();
    bool lightExist_;
    bool vibratorExist_;
    std::vector<LightInfoIPC> lightInfos_;
    std::map<MiscdeviceDeviceId, bool> miscDeviceIdMap_;
    MiscdeviceServiceState state_;
    std::mutex vibratorThreadMutex_;
    sptr<IRemoteObject::DeathRecipient> clientDeathObserver_ = nullptr;
    std::mutex clientDeathObserverMutex_;
    static std::map<sptr<IRemoteObject>, int32_t> clientPidMap_;
    std::mutex clientPidMapMutex_;
    std::mutex miscDeviceIdMapMutex_;
    std::mutex lightInfosMutex_;
    std::mutex devicesManageMutex_;
    static std::map<int32_t, VibratorAllInfos> devicesManageMap_;
    int32_t invalidVibratorIdCount_ = 0;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // MISCDEVICE_SERVICE_H
