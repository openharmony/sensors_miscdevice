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
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "MiscdeviceServiceProxy" };
constexpr int32_t MAX_VIBRATOR_COUNT = 0XFF;
constexpr int32_t MAX_LIGHT_COUNT = 0XFF;
}

MiscdeviceServiceProxy::MiscdeviceServiceProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IMiscdeviceService>(impl)
{}

bool MiscdeviceServiceProxy::IsAbilityAvailable(MiscdeviceDeviceId groupID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return false;
    }
    if (!data.WriteUint32(static_cast<uint32_t>(groupID))) {
        MISC_HILOGE("write groupID failed");
        return false;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(IS_ABILITY_AVAILABLE, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "IsAbilityAvailable", "ERROR_CODE", ret);
        MISC_HILOGE("sendRequest failed, ret : %{public}d", ret);
        return false;
    }
    bool isAbilityAvailable;
    if (!reply.ReadBool(isAbilityAvailable)) {
        MISC_HILOGE("Parcel read failed");
        return false;
    }
    return isAbilityAvailable;
}

bool MiscdeviceServiceProxy::IsVibratorEffectAvailable(int32_t vibratorId, const std::string &effectType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return false;
    }
    if (!data.WriteInt32(vibratorId)) {
        MISC_HILOGE("WriteInt32 failed");
        return false;
    }
    if (!data.WriteString(effectType)) {
        MISC_HILOGE("WriteString failed");
        return false;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(IS_VIBRATOR_EFFECT_AVAILABLE, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "IsVibratorEffectAvailable", "ERROR_CODE", ret);
        MISC_HILOGE("SendRequest failed, ret : %{public}d", ret);
        return false;
    }
    bool isVibratorEffectAvailable;
    if (!reply.ReadBool(isVibratorEffectAvailable)) {
        MISC_HILOGE("Parcel read failed");
        return false;
    }
    return isVibratorEffectAvailable;
}

std::vector<int32_t> MiscdeviceServiceProxy::GetVibratorIdList()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<int32_t> idVec;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return idVec;
    }
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        MISC_HILOGE("remote is null");
        return idVec;
    }
    int32_t ret = remote->SendRequest(GET_VIBRATOR_ID_LIST, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "GetVibratorIdList", "ERROR_CODE", ret);
        MISC_HILOGE("SendRequest failed, ret : %{public}d", ret);
        return idVec;
    }
    uint32_t setCount;
    if (!reply.ReadUint32(setCount)) {
        MISC_HILOGE("Parcel read failed");
        return idVec;
    }
    if (setCount == 0 || setCount > MAX_VIBRATOR_COUNT) {
        MISC_HILOGE("setCount: %{public}d is invalid", setCount);
        return idVec;
    }
    idVec.resize(setCount);
    reply.ReadInt32Vector(&idVec);
    return idVec;
}

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
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
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
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
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
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "PlayVibratorEffect", "ERROR_CODE", ret);
        MISC_HILOGE("ret : %{public}d", ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::PlayCustomVibratorEffect(int32_t vibratorId, const std::vector<int32_t> &timing,
                                                         const std::vector<int32_t> &intensity, int32_t periodCount)
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
    if (!data.WriteInt32Vector(timing)) {
        MISC_HILOGE("WriteInt32Vector timing failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32Vector(intensity)) {
        MISC_HILOGE("WriteInt32Vector intensity failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(periodCount)) {
        MISC_HILOGE("WriteInt32 periodCount failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(PLAY_CUSTOM_VIBRATOR_EFFECT, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "PlayCustomVibratorEffect", "ERROR_CODE", ret);
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
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "StopVibratorEffect", "ERROR_CODE", ret);
        MISC_HILOGE("ret : %{public}d", ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::SetVibratorParameter(int32_t vibratorId, const std::string &cmd)
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
    if (!data.WriteString(cmd)) {
        MISC_HILOGE("WriteString cmd failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(SET_VIBRATOR_PARA, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "SetVibratorParameter", "ERROR_CODE", ret);
        MISC_HILOGE("ret : %{public}d", ret);
    }
    return ret;
}

std::string MiscdeviceServiceProxy::GetVibratorParameter(int32_t vibratorId, const std::string &cmd)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return "";
    }
    if (!data.WriteInt32(vibratorId)) {
        MISC_HILOGE("WriteInt32 vibratorId failed");
        return "";
    }
    if (!data.WriteString(cmd)) {
        MISC_HILOGE("WriteString cmd failed");
        return "";
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPS(remote);
    int32_t ret = remote->SendRequest(GET_VIBRATOR_PARA, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "GetVibratorParameter", "ERROR_CODE", ret);
        MISC_HILOGE("ret : %{public}d", ret);
        return "";
    }
    std::string vibratorParameter;
    if (!reply.ReadString(vibratorParameter)) {
        MISC_HILOGE("Parcel read failed");
        return "";
    }
    return vibratorParameter;
}

std::vector<int32_t> MiscdeviceServiceProxy::GetLightSupportId()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<int32_t> idVec;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return idVec;
    }
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        MISC_HILOGE("remote is null");
        return idVec;
    }
    int32_t ret = remote->SendRequest(GET_LIGHT_SUPPORT_ID, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "GetLightSupportId", "ERROR_CODE", ret);
        MISC_HILOGE("sendRequest failed, ret : %{public}d", ret);
    }
    uint32_t setCount;
    if (!reply.ReadUint32(setCount)) {
        MISC_HILOGE("Parcel read failed");
        return idVec;
    }
    if (setCount == 0 || setCount > MAX_LIGHT_COUNT) {
        MISC_HILOGE("setCount: %{public}d is invalid", setCount);
        return idVec;
    }
    idVec.resize(setCount);
    reply.ReadInt32Vector(&idVec);
    return idVec;
}

bool MiscdeviceServiceProxy::IsLightEffectSupport(int32_t lightId, const std::string &effectId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return false;
    }
    if (!data.WriteInt32(lightId)) {
        MISC_HILOGE("WriteInt32 lightId failed");
        return false;
    }
    if (!data.WriteString(effectId)) {
        MISC_HILOGE("WriteString effectId failed");
        return false;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPF(remote);
    int32_t ret = remote->SendRequest(IS_LIGHT_EFFECT_SUPPORT, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "IsLightEffectSupport", "ERROR_CODE", ret);
        MISC_HILOGE("sendRequest failed, ret : %{public}d", ret);
        return false;
    }
    bool isLightEffectSupport;
    if (!reply.ReadBool(isLightEffectSupport)) {
        MISC_HILOGE("Parcel read failed");
        return false;
    }
    return isLightEffectSupport;
}

int32_t MiscdeviceServiceProxy::Light(int32_t id, uint64_t brightness, uint32_t timeOn, uint32_t timeOff)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(id)) {
        MISC_HILOGE("WriteInt32 id failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteUint64(brightness)) {
        MISC_HILOGE("WriteUint64 brightness failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteUint32(timeOn)) {
        MISC_HILOGE("WriteUint32 timeOn failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteUint32(timeOff)) {
        MISC_HILOGE("WriteUint32 timeOff failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(LIGHT, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "Light", "ERROR_CODE", ret);
        MISC_HILOGE("sendRequest failed, ret : %{public}d", ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::PlayLightEffect(int32_t id, const std::string &type)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(id)) {
        MISC_HILOGE("WriteInt32 id failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteString(type)) {
        MISC_HILOGE("WriteString type failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(PLAY_LIGHT_EFFECT, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "PlayLightEffect", "ERROR_CODE", ret);
        MISC_HILOGE("sendRequest failed, ret : %{public}d", ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::StopLightEffect(int32_t id)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        MISC_HILOGE("write descriptor failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(id)) {
        MISC_HILOGE("WriteInt32 id failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(STOP_LIGHT_EFFECT, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "StopLightEffect", "ERROR_CODE", ret);
        MISC_HILOGE("sendRequest failed, ret : %{public}d", ret);
    }
    return ret;
}
}  // namespace Sensors
}  // namespace OHOS
