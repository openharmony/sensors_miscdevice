/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "vibrator_client_stub.h"

#include "message_parcel.h"

#include "miscdevice_log.h"
#include "vibrator_service_client.h"

#undef LOG_TAG
#define LOG_TAG "VibratorClientStub"
namespace OHOS {
namespace Sensors {
namespace {
    constexpr int32_t TIME_CONVERSION_UNIT { 1000 };
} //unnamespace
using OHOS::Sensors::VibratorServiceClient;

int32_t VibratorClientStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    std::u16string descriptor = VibratorClientStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        MISC_HILOGE("Client and service descriptors are inconsistent");
        return OBJECT_NULL;
    }
	MISC_HILOGD("Begin, cmd:%{public}u", code);
    switch (code) {
        case TRANS_ID_PLUG_ABILITY: {
            int32_t eventCode = 0;
            int32_t deviceId = -1;
            int32_t vibratorCnt = 0;
            if (!data.ReadInt32(eventCode)) {
                MISC_HILOGE("Read eventCode failed.");
                return PARAMETER_ERROR;
            }
            if (!data.ReadInt32(deviceId)) {
                MISC_HILOGE("Read deviceId failed.");
                return PARAMETER_ERROR;
            }
            if (!data.ReadInt32(vibratorCnt)) {
                MISC_HILOGE("Read vibratorCnt failed.");
                return PARAMETER_ERROR;
            }
            int result = ProcessPlugEvent(eventCode, deviceId, vibratorCnt);
            reply.WriteInt32(result);
            return NO_ERROR;
        }
    }
    return NO_ERROR;
}

int64_t VibratorClientStub::GetSystemTime()
{
    struct timespec ts = { 0, 0 };
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        MMI_HILOGD("clock_gettime failed:%{public}d", errno);
        return 0;
    }
    return (ts.tv_sec * TIME_CONVERSION_UNIT * TIME_CONVERSION_UNIT) + (ts.tv_nsec / TIME_CONVERSION_UNIT);
}

int VibratorClientStub::ProcessPlugEvent(int32_t eventCode, int32_t deviceId, int32_t vibratorCnt)
{
    MISC_HILOGD("Begin, eventCode=%{public}d, deviceId:%{public}d, vibratorCnt:%{public}d",
        eventCode, deviceId, vibratorCnt);
    VibratorDeviceInfo info = {
        .type = static_cast<VibratorPlugState>(eventCode),
        .deviceId = deviceId,
        .timestamp = GetSystemTime(),
        .vibratorCnt = vibratorCnt
    };
    auto &client = VibratorServiceClient::GetInstance();
    bool ret = client.HandleVibratorData(info);
    if (!ret) {
        MISC_HILOGE("Handle bibrator data failed, ret:%{public}d", ret);
        return PARAMETER_ERROR;
    }
    MISC_HILOGD("Success to process plug event");
    return NO_ERROR;
}
} // namespace Sensors
} // namespace OHOS