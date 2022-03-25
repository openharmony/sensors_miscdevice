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

#ifndef VIBRATOR_SERVICE_CLIENT_H
#define VIBRATOR_SERVICE_CLIENT_H

#include <mutex>
#include "iremote_object.h"
#include "miscdevice_service_proxy.h"
#include "singleton.h"

namespace OHOS {
namespace Sensors {
class VibratorServiceClient : public Singleton<VibratorServiceClient> {
public:
    std::vector<int32_t> GetVibratorIdList();
    bool IsVibratorEffectSupport(int32_t vibratorId, const std::string &effect);
    int32_t Vibrate(int32_t vibratorId, uint32_t timeOut);
    int32_t Vibrate(int32_t vibratorId, const std::string &effect, bool isLooping);
    int32_t Vibrate(int32_t vibratorId, std::vector<int32_t> timing, std::vector<int32_t> intensity,
                    int32_t periodCount);
    int32_t Stop(int32_t vibratorId, const std::string &type);
    int32_t SetVibratorParameter(int32_t vibratorId, const std::string &cmd);
    std::string GetVibratorParameter(int32_t vibratorId, const std::string &command);
    void ProcessDeathObserver(const wptr<IRemoteObject> &object);

private:
    int32_t InitServiceClient();
    sptr<IRemoteObject::DeathRecipient> serviceDeathObserver_ = nullptr;
    sptr<IMiscdeviceService> miscdeviceProxy_ = nullptr;
    std::mutex clientMutex_;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // VIBRATOR_SERVICE_CLIENT_H
