/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "accesstoken_kit.h"
#include "nocopyable.h"
#include "system_ability.h"
#include "thread_ex.h"

#include "file_utils.h"
#include "json_parser.h"
#include "light_hdi_connection.h"
#include "miscdevice_common.h"
#include "miscdevice_delayed_sp_singleton.h"
#include "miscdevice_dump.h"
#include "miscdevice_service_stub.h"
#include "vibrator_hdi_connection.h"
#include "vibrator_infos.h"
#include "vibrator_thread.h"

namespace OHOS {
namespace Sensors {
using namespace Security::AccessToken;
enum class MiscdeviceServiceState {
    STATE_STOPPED,
    STATE_RUNNING,
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
    bool IsLightAnimationValid(const LightAnimation &animation);
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args) override;
    virtual int32_t Vibrate(int32_t vibratorId, int32_t timeOut, int32_t usage) override;
    virtual int32_t PlayVibratorEffect(int32_t vibratorId, const std::string &effect,
                                       int32_t loopCount, int32_t usage) override;
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    virtual int32_t PlayVibratorCustom(int32_t vibratorId, const RawFileDescriptor &rawFd, int32_t usage) override;
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    virtual int32_t StopVibrator(int32_t vibratorId) override;
    virtual int32_t StopVibrator(int32_t vibratorId, const std::string &mode) override;
    virtual int32_t IsSupportEffect(const std::string &effect, bool &state) override;
    virtual std::vector<LightInfo> GetLightList() override;
    virtual int32_t TurnOn(int32_t lightId, const LightColor &color, const LightAnimation &animation) override;
    virtual int32_t TurnOff(int32_t lightId) override;

private:
    DISALLOW_COPY_AND_MOVE(MiscdeviceService);
    bool InitInterface();
    bool InitLightInterface();
    std::string GetPackageName(AccessTokenID tokenId);
    void StartVibrateThread(VibrateInfo info);
    bool ShouldIgnoreVibrate(const VibrateInfo &info);
    bool InitLightList();
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    int32_t DecodeCustomEffect(const RawFileDescriptor &rawFd, std::set<VibrateEvent> &vibrateSequence);
    int32_t StartCustomVibration(const RawFileDescriptor &rawFd, const VibrateInfo &info);
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    VibratorHdiConnection &vibratorHdiConnection_ = VibratorHdiConnection::GetInstance();
    LightHdiConnection &lightHdiConnection_ = LightHdiConnection::GetInstance();
    bool lightExist_;
    bool vibratorExist_;
    std::vector<LightInfo> lightInfos_;
    std::map<MiscdeviceDeviceId, bool> miscDeviceIdMap_;
    MiscdeviceServiceState state_;
    std::shared_ptr<VibratorThread> vibratorThread_;
    std::mutex vibratorThreadMutex_;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // MISCDEVICE_SERVICE_H
