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

#ifndef MISCDEVICE_SERVICE_PROXY_H
#define MISCDEVICE_SERVICE_PROXY_H

#include "i_miscdevice_service.h"
#include "iremote_proxy.h"
#include "nocopyable.h"

namespace OHOS {
namespace Sensors {
class MiscdeviceServiceProxy : public IRemoteProxy<IMiscdeviceService> {
public:
    explicit MiscdeviceServiceProxy(const sptr<IRemoteObject> &impl);

    ~MiscdeviceServiceProxy() = default;

    virtual int32_t Vibrate(int32_t vibratorId, int32_t timeOut, int32_t usage) override;

    virtual int32_t CancelVibrator(int32_t vibratorId) override;

    virtual int32_t PlayVibratorEffect(int32_t vibratorId, const std::string &effect,
	                                   int32_t loopCount, int32_t usage) override;

    virtual int32_t StopVibratorEffect(int32_t vibratorId, const std::string &effect) override;

private:
    DISALLOW_COPY_AND_MOVE(MiscdeviceServiceProxy);
    static inline BrokerDelegator<MiscdeviceServiceProxy> delegator_;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // MISCDEVICE_SERVICE_PROXY_H
