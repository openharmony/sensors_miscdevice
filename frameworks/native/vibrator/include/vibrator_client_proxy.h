/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef VIBRATOR_CLIENT_STUB_H
#define VIBRATOR_CLIENT_STUB_H

#include "message_parcel.h"
#include "miscdevice_log.h"
#include "iremote_proxy.h"
#include "i_vibrator_client.h"

#undef LOG_TAG
#define LOG_TAG "VibratorClientProxy"

namespace OHOS {
namespace Sensors {
class VibratorClientProxy : public IRemoteProxy<IVibratorClient> {
public:
    explicit VibratorClientProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IVibratorClient>(impl) {}
    virtual ~VibratorClientProxy() = default;
    int ProcessPlugEvent(int32_t eventCode, int32_t deviceId, int32_t vibratorCnt) override
    {
        MessageOption option;
        MessageParcel dataParcel;
        MessageParcel replyParcel;
        
        if (!dataParcel.WriteInterfaceToken(GetDescriptor())) {
            MISC_HILOGE("Failed to write descriptor to parcelable");
            return PARAMETER_ERROR;
        }
        if (!dataParcel.WriteInt32(eventCode)) {
            MISC_HILOGE("Failed to write eventCode to parcelable");
            return PARAMETER_ERROR;
        }
        if (!dataParcel.WriteInt32(deviceId)) {
            MISC_HILOGE("Failed to write deviceId to parcelable");
            return PARAMETER_ERROR;
        }
        if (!dataParcel.WriteInt32(vibratorCnt)) {
            MISC_HILOGE("Failed to write vibratorCnt to parcelable");
            return PARAMETER_ERROR;
        }
        if (Remote() == nullptr) {
            MISC_HILOGE("Remote is nullptr");
            return ERROR;
        }
        int error = Remote()->SendRequest(TRANS_ID_PLUG_ABILITY, dataParcel, replyParcel, option);
        if (error != ERR_NONE) {
            MISC_HILOGE("failed, error code is: %{public}d", error);
            return PARAMETER_ERROR;
        }
        return replyParcel.ReadInt32();
    }

private:
    DISALLOW_COPY_AND_MOVE(VibratorClientProxy);
    static inline BrokerDelegator<VibratorClientProxy> delegator_;
};
} // namespace Sensors
} // namespace OHOS
#endif // VIBRATOR_CLIENT_STUB_H
