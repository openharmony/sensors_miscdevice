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

#ifndef MISCDEVICE_SERVICE_STUB_H
#define MISCDEVICE_SERVICE_STUB_H

#include <map>

#include "iremote_stub.h"
#include "message_parcel.h"
#include "nocopyable.h"

#include "i_miscdevice_service.h"

namespace OHOS {
namespace Sensors {
using ServicePb = std::function<int32_t(uint32_t code, MessageParcel &, MessageParcel &, MessageOption &)>;

class MiscdeviceServiceStub : public IRemoteStub<IMiscdeviceService> {
public:
    MiscdeviceServiceStub();

    virtual ~MiscdeviceServiceStub();

    virtual int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                    MessageOption &option) override;

private:
    DISALLOW_COPY_AND_MOVE(MiscdeviceServiceStub);
    using MiscBaseFunc = int32_t (MiscdeviceServiceStub::*)(MessageParcel &data, MessageParcel &reply);

    int32_t IsAbilityAvailablePb(MessageParcel &data, MessageParcel &reply);
    int32_t IsVibratorEffectAvailablePb(MessageParcel &data, MessageParcel &reply);
    int32_t GetVibratorIdListPb(MessageParcel &data, MessageParcel &reply);
    int32_t VibratePb(MessageParcel &data, MessageParcel &reply);
    int32_t CancelVibratorPb(MessageParcel &data, MessageParcel &reply);
    int32_t PlayVibratorEffectPb(MessageParcel &data, MessageParcel &reply);
    int32_t PlayCustomVibratorEffectPb(MessageParcel &data, MessageParcel &reply);
    int32_t StopVibratorEffectPb(MessageParcel &data, MessageParcel &reply);
    int32_t SetVibratorParameterPb(MessageParcel &data, MessageParcel &reply);
    int32_t GetVibratorParameterPb(MessageParcel &data, MessageParcel &reply);
    int32_t GetLightListPb(MessageParcel &data, MessageParcel &reply);
    int32_t TurnOnPb(MessageParcel &data, MessageParcel &reply);
    int32_t TurnOffPb(MessageParcel &data, MessageParcel &reply);
    bool CheckVibratePermission();

    std::map<uint32_t, MiscBaseFunc> baseFuncs_;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // MISCDEVICE_SERVICE_STUB_H
