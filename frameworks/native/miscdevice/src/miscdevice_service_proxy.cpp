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

#include "miscdevice_service_proxy.h"

#include "hisysevent.h"
#include "securec.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "MiscdeviceServiceProxy" };
constexpr int32_t MAX_LIGHT_COUNT = 0XFF;
}

MiscdeviceServiceProxy::MiscdeviceServiceProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IMiscdeviceService>(impl)
{}

int32_t MiscdeviceServiceProxy::Vibrate(int32_t vibratorId, int32_t timeOut, int32_t usage)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(vibratorId)) {
        MISC_HILOGE("WriteInt32 vibratorId failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(timeOut)) {
        MISC_HILOGE("WriteUint32 timeOut failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(usage)) {
        MISC_HILOGE("WriteUint32 usage failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(VIBRATE, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "Vibrate", "ERROR_CODE", ret);
        MISC_HILOGE("sendRequest ret : %{public}d", ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::CancelVibrator(int32_t vibratorId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(vibratorId)) {
        MISC_HILOGE("WriteInt32 failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(CANCEL_VIBRATOR, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "CancelVibrator", "ERROR_CODE", ret);
        MISC_HILOGE("ret : %{public}d", ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::PlayVibratorEffect(int32_t vibratorId, const std::string &effect,
    int32_t loopCount, int32_t usage)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(vibratorId)) {
        MISC_HILOGE("WriteInt32 vibratorId failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteString(effect)) {
        MISC_HILOGE("WriteString effect failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(loopCount)) {
        MISC_HILOGE("WriteBool effect failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(usage)) {
        MISC_HILOGE("WriteUint32 usage failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(PLAY_VIBRATOR_EFFECT, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "PlayVibratorEffect", "ERROR_CODE", ret);
        MISC_HILOGE("ret : %{public}d", ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::StopVibratorEffect(int32_t vibratorId, const std::string &effect)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(vibratorId)) {
        MISC_HILOGE("WriteInt32 vibratorId failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteString(effect)) {
        MISC_HILOGE("WriteString effect failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(STOP_VIBRATOR_EFFECT, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "StopVibratorEffect", "ERROR_CODE", ret);
        MISC_HILOGE("ret : %{public}d", ret);
    }
    return ret;
}
std::vector<LightInfo> MiscdeviceServiceProxy::GetLightList()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<LightInfo> lightInfos;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("WriteInterfaceToken failed");
        return lightInfos;
    }
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        MISC_HILOGE("remote is null");
        return lightInfos;
    }
    int32_t ret = remote->SendRequest(GET_LIGHT_LIST, data, reply, option);
    if (ret != NO_ERROR) {
        MISC_HILOGE("failed, ret:%{public}d", ret);
        return lightInfos;
    }
    uint32_t lightCount = 0;
    if (!reply.ReadUint32(lightCount)) {
        MISC_HILOGE("Parcel read failed");
        return lightInfos;
    }
    if (lightCount > MAX_LIGHT_COUNT) {
        lightCount = MAX_LIGHT_COUNT;
    }
    for (uint32_t i = 0; i < lightCount; ++i) {
        const uint8_t *info = reply.ReadBuffer(sizeof(LightInfo));
        if (info == nullptr) {
            MISC_HILOGE("ReadBuffer failed");
            return lightInfos;
        }
        LightInfo lightInfo;
        if (memcpy_s(&lightInfo, sizeof(LightInfo), info, sizeof(LightInfo)) != EOK) {
            MISC_HILOGE("memcpy_s failed");
            return lightInfos;
        }
        lightInfos.push_back(lightInfo);
    }
    return lightInfos;
}

int32_t MiscdeviceServiceProxy::TurnOn(int32_t lightId, const LightColor &color, const LightAnimation &animation)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(lightId)) {
        MISC_HILOGE("WriteUint32 lightId failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteBuffer(&color, sizeof(LightColor))) {
        MISC_HILOGE("WriteBuffer color failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteBuffer(&animation, sizeof(LightAnimation))) {
        MISC_HILOGE("WriteBuffer animation failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(TURN_ON, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "TurnOn", "ERROR_CODE", ret);
        MISC_HILOGE("sendRequest failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::TurnOff(int32_t lightId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(lightId)) {
        MISC_HILOGE("WriteInt32 lightId failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(TURN_OFF, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "TurnOff", "ERROR_CODE", ret);
        MISC_HILOGE("sendRequest failed, ret:%{public}d", ret);
    }
    return ret;
}
}  // namespace Sensors
}  // namespace OHOS
