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

#include "dmd_report.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::MISCDEVICE_SERVICE, "MiscdeviceServiceProxy" };
}

MiscdeviceServiceProxy::MiscdeviceServiceProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IMiscdeviceService>(impl)
{}

bool MiscdeviceServiceProxy::IsAbilityAvailable(MiscdeviceDeviceId groupID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return false;
    }
    if (!data.WriteUint32(static_cast<uint32_t>(groupID))) {
        HiLog::Error(LABEL, "%{public}s write groupID failed", __func__);
        return false;
    }
    int32_t ret = Remote()->SendRequest(IS_ABILITY_AVAILABLE, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "IsAbilityAvailable", ret);
        HiLog::Error(LABEL, "%{public}s failed, ret : %{public}d", __func__, ret);
        return false;
    }
    return reply.ReadBool();
}

bool MiscdeviceServiceProxy::IsVibratorEffectAvailable(int32_t vibratorId, const std::string &effectType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return false;
    }
    if (!data.WriteInt32(vibratorId)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32 failed", __func__);
        return false;
    }
    if (!data.WriteString(effectType)) {
        HiLog::Error(LABEL, "%{public}s WriteString failed", __func__);
        return false;
    }
    int32_t ret = Remote()->SendRequest(IS_VIBRATOR_EFFECT_AVAILABLE, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "IsVibratorEffectAvailable", ret);
        HiLog::Error(LABEL, "%{public}s failed, ret : %{public}d", __func__, ret);
        return false;
    }
    return reply.ReadBool();
}

std::vector<int32_t> MiscdeviceServiceProxy::GetVibratorIdList()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<int32_t> idVec;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return idVec;
    }
    int32_t ret = Remote()->SendRequest(GET_VIBRATOR_ID_LIST, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "GetVibratorIdList", ret);
        HiLog::Error(LABEL, "%{public}s failed, ret : %{public}d", __func__, ret);
        return idVec;
    }
    uint32_t setCount = reply.ReadUint32();
    if (setCount <= 0 || setCount > idVec.max_size()) {
        HiLog::Error(LABEL, "%{public}s setCount: %{public}d is invalid", __func__, setCount);
        return idVec;
    }
    idVec.resize(setCount);
    reply.ReadInt32Vector(&idVec);
    return idVec;
}

int32_t MiscdeviceServiceProxy::Vibrate(int32_t vibratorId, uint32_t timeOut)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(vibratorId)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32 vibratorId failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteUint32(timeOut)) {
        HiLog::Error(LABEL, "%{public}s WriteUint32 timeOut failed", __func__);
        return WRITE_MSG_ERR;
    }
    int32_t ret = Remote()->SendRequest(VIBRATE, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "Vibrate", ret);
        HiLog::Error(LABEL, "%{public}s ret : %{public}d", __func__, ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::CancelVibrator(int32_t vibratorId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(vibratorId)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32 failed", __func__);
        return WRITE_MSG_ERR;
    }
    int32_t ret = Remote()->SendRequest(CANCEL_VIBRATOR, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "CancelVibrator", ret);
        HiLog::Error(LABEL, "%{public}s ret : %{public}d", __func__, ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::PlayVibratorEffect(int32_t vibratorId, const std::string &effect, bool isLooping)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(vibratorId)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32 vibratorId failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteString(effect)) {
        HiLog::Error(LABEL, "%{public}s WriteString effect failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteBool(isLooping)) {
        HiLog::Error(LABEL, "%{public}s WriteBool effect failed", __func__);
        return WRITE_MSG_ERR;
    }
    int32_t ret = Remote()->SendRequest(PLAY_VIBRATOR_EFFECT, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "PlayVibratorEffect", ret);
        HiLog::Error(LABEL, "%{public}s ret : %{public}d", __func__, ret);
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
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(vibratorId)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32 vibratorId failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32Vector(timing)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32Vector timing failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32Vector(intensity)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32Vector intensity failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(periodCount)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32 periodCount failed", __func__);
        return WRITE_MSG_ERR;
    }
    int32_t ret = Remote()->SendRequest(PLAY_CUSTOM_VIBRATOR_EFFECT, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "PlayCustomVibratorEffect", ret);
        HiLog::Error(LABEL, "%{public}s ret : %{public}d", __func__, ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::StopVibratorEffect(int32_t vibratorId, const std::string &effect)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(vibratorId)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32 vibratorId failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteString(effect)) {
        HiLog::Error(LABEL, "%{public}s WriteString effect failed", __func__);
        return WRITE_MSG_ERR;
    }
    int32_t ret = Remote()->SendRequest(STOP_VIBRATOR_EFFECT, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "StopVibratorEffect", ret);
        HiLog::Error(LABEL, "%{public}s ret : %{public}d", __func__, ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::SetVibratorParameter(int32_t vibratorId, const std::string &cmd)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(vibratorId)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32 vibratorId failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteString(cmd)) {
        HiLog::Error(LABEL, "%{public}s  WriteString cmd failed", __func__);
        return WRITE_MSG_ERR;
    }
    int32_t ret = Remote()->SendRequest(SET_VIBRATOR_PARA, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "SetVibratorParameter", ret);
        HiLog::Error(LABEL, "%{public}s ret : %{public}d", __func__, ret);
    }
    return ret;
}

std::string MiscdeviceServiceProxy::GetVibratorParameter(int32_t vibratorId, const std::string &cmd)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return "";
    }
    if (!data.WriteInt32(vibratorId)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32 vibratorId failed", __func__);
        return "";
    }
    if (!data.WriteString(cmd)) {
        HiLog::Error(LABEL, "%{public}s WriteString cmd failed", __func__);
        return "";
    }
    int32_t ret = Remote()->SendRequest(GET_VIBRATOR_PARA, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "SetVibratorParameter", ret);
        HiLog::Error(LABEL, "%{public}s ret : %{public}d", __func__, ret);
        return "";
    }
    return reply.ReadString();
}

std::vector<int32_t> MiscdeviceServiceProxy::GetLightSupportId()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<int32_t> idVec;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return idVec;
    }
    int32_t ret = Remote()->SendRequest(GET_LIGHT_SUPPORT_ID, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "GetLightSupportId", ret);
        HiLog::Error(LABEL, "%{public}s failed, ret : %{public}d", __func__, ret);
    }
    int32_t setCount = reply.ReadInt32();
    if (setCount <= 0 || setCount > idVec.max_size()) {
        HiLog::Error(LABEL, "%{public}s setCount: %{public}d is invalid", __func__, setCount);
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
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return false;
    }
    if (!data.WriteInt32(lightId)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32 lightId failed", __func__);
        return false;
    }
    if (!data.WriteString(effectId)) {
        HiLog::Error(LABEL, "%{public}s WriteString effectId failed", __func__);
        return false;
    }

    int32_t ret = Remote()->SendRequest(IS_LIGHT_EFFECT_SUPPORT, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "IsLightEffectSupport", ret);
        HiLog::Error(LABEL, "%{public}s failed, ret : %{public}d", __func__, ret);
        return false;
    }
    return reply.ReadBool();
}

int32_t MiscdeviceServiceProxy::Light(int32_t id, uint64_t brightness, uint32_t timeOn, uint32_t timeOff)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(id)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32 id failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteUint64(brightness)) {
        HiLog::Error(LABEL, "%{public}s WriteUint64 brightness failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteUint32(timeOn)) {
        HiLog::Error(LABEL, "%{public}s WriteUint32 timeOn failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteUint32(timeOff)) {
        HiLog::Error(LABEL, "%{public}s WriteUint32 timeOff failed", __func__);
        return WRITE_MSG_ERR;
    }

    int32_t ret = Remote()->SendRequest(LIGHT, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "Light", ret);
        HiLog::Error(LABEL, "%{public}s failed, ret : %{public}d", __func__, ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::PlayLightEffect(int32_t id, const std::string &type)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(id)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32 id failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteString(type)) {
        HiLog::Error(LABEL, "%{public}s WriteString type failed", __func__);
        return WRITE_MSG_ERR;
    }
    int32_t ret = Remote()->SendRequest(PLAY_LIGHT_EFFECT, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "PlayLightEffect", ret);
        HiLog::Error(LABEL, "%{public}s failed, ret : %{public}d", __func__, ret);
    }
    return ret;
}

int32_t MiscdeviceServiceProxy::StopLightEffect(int32_t id)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(MiscdeviceServiceProxy::GetDescriptor())) {
        HiLog::Error(LABEL, "%{public}s write descriptor failed", __func__);
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt32(id)) {
        HiLog::Error(LABEL, "%{public}s WriteInt32 id failed", __func__);
        return WRITE_MSG_ERR;
    }
    int32_t ret = Remote()->SendRequest(STOP_LIGHT_EFFECT, data, reply, option);
    if (ret != NO_ERROR) {
        DmdReport::ReportException(MISC_SERVICE_IPC_EXCEPTION, "StopLightEffect", ret);
        HiLog::Error(LABEL, "%{public}s failed, ret : %{public}d", __func__, ret);
    }
    return ret;
}
}  // namespace Sensors
}  // namespace OHOS
