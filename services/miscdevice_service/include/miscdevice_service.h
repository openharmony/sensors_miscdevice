/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <condition_variable>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "system_ability.h"
#include "thread_ex.h"

#include "miscdevice_common.h"
#include "miscdevice_dump.h"
#include "miscdevice_service_stub.h"
#include "nocopyable.h"
#include "vibrator_hdi_connection.h"

namespace OHOS {
namespace Sensors {
enum class MiscdeviceServiceState {
    STATE_STOPPED,
    STATE_RUNNING,
};

class MiscdeviceService : public SystemAbility, public MiscdeviceServiceStub {
    DECLARE_SYSTEM_ABILITY(MiscdeviceService)
public:
    explicit MiscdeviceService(int32_t systemAbilityId, bool runOnCreate = false);
    ~MiscdeviceService();
    void OnDump() override;
    void OnStart() override;
    void OnStop() override;
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args) override;
    virtual bool IsAbilityAvailable(MiscdeviceDeviceId groupID) override;
    virtual bool IsVibratorEffectAvailable(int32_t vibratorId, const std::string &effectType) override;
    virtual std::vector<int32_t> GetVibratorIdList() override;
    virtual int32_t Vibrate(int32_t vibratorId, uint32_t timeOut) override;
    virtual int32_t CancelVibrator(int32_t vibratorId) override;
    virtual int32_t PlayVibratorEffect(int32_t vibratorId, const std::string &effect, bool isLooping) override;
    virtual int32_t PlayCustomVibratorEffect(int32_t vibratorId, const std::vector<int32_t> &timing,
                                             const std::vector<int32_t> &intensity, int32_t periodCount) override;
    virtual int32_t StopVibratorEffect(int32_t vibratorId, const std::string &effect) override;
    virtual int32_t SetVibratorParameter(int32_t vibratorId, const std::string &cmd) override;
    virtual std::string GetVibratorParameter(int32_t vibratorId, const std::string &cmd) override;
    virtual std::vector<int32_t> GetLightSupportId() override;
    virtual bool IsLightEffectSupport(int32_t lightId, const std::string &effectId) override;
    virtual int32_t Light(int32_t lightId, uint64_t brightness, uint32_t timeOn, uint32_t timeOff) override;
    virtual int32_t PlayLightEffect(int32_t lightId, const std::string &type) override;
    virtual int32_t StopLightEffect(int32_t lightId) override;

private:
    DISALLOW_COPY_AND_MOVE(MiscdeviceService);
    bool InitInterface();
    VibratorHdiConnection &vibratorHdiConnection_ = VibratorHdiConnection::GetInstance();
    class VibratorThread : public Thread {
    public:
        explicit VibratorThread(const MiscdeviceService &service);
        ~VibratorThread() = default;
        void UpdateVibratorData(const std::vector<int32_t> &timing, const std::vector<int32_t> &intensity,
                                int32_t &periodCount);
        void NotifyExit();

    protected:
        virtual bool Run();

    private:
        std::timed_mutex mtx_;
        std::vector<int32_t> vecTimingMs_;
        std::vector<int32_t> intensitys_;
        int32_t periodCount_;
    };
    class VibratorEffectThread : public Thread {
    public:
        explicit VibratorEffectThread(const MiscdeviceService &service);
        ~VibratorEffectThread() = default;
        void UpdateVibratorEffectData(const std::string effect, int32_t delayTiming);

    protected:
        virtual bool Run();

    private:
        std::string hapticEffect_;
        int32_t delayTimingMs_;
    };
    bool lightExist_;
    bool vibratorExist_;
    std::set<LightId> lightSupportId_;
    std::map<MiscdeviceDeviceId, bool> miscDdeviceIdMap_;
    MiscdeviceServiceState state_;
    VibratorThread *vibratorThread_ = nullptr;
    std::unique_ptr<VibratorEffectThread> vibratorEffectThread_;
    std::mutex vibratorThreadMutex_;
    std::mutex vibratorEffectMutex_;
    std::mutex vibratorEffectThreadMutex_;
    std::map<int32_t, std::string> vibratorEffectMap_;
    static bool ready_;
    static std::mutex conditionVarMutex_;
    static std::condition_variable conditionVar_;
    static std::unordered_map<std::string, int32_t> hapticRingMap_;
    MiscdeviceDump &miscdeviceDump_ = MiscdeviceDump::GetInstance();
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // MISCDEVICE_SERVICE_H
