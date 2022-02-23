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

#include "miscdevice_service_stub.h"

#include <string>
#include <unistd.h>

#include "ipc_skeleton.h"
#include "message_parcel.h"

#include "permission_util.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::MISCDEVICE_SERVICE, "MiscdeviceServiceStub" };
const std::string VIBRATE_PERMISSION = "ohos.permission.VIBRATE";
}  // namespace

MiscdeviceServiceStub::MiscdeviceServiceStub()
{
    HiLog::Info(LABEL, "%{public}s begin,  %{public}p", __func__, this);
    baseFuncs_[IS_ABILITY_AVAILABLE] = &MiscdeviceServiceStub::IsAbilityAvailablePb;
    baseFuncs_[IS_VIBRATOR_EFFECT_AVAILABLE] = &MiscdeviceServiceStub::IsVibratorEffectAvailablePb;
    baseFuncs_[GET_VIBRATOR_ID_LIST] = &MiscdeviceServiceStub::GetVibratorIdListPb;
    baseFuncs_[VIBRATE] = &MiscdeviceServiceStub::VibratePb;
    baseFuncs_[CANCEL_VIBRATOR] = &MiscdeviceServiceStub::CancelVibratorPb;
    baseFuncs_[PLAY_VIBRATOR_EFFECT] = &MiscdeviceServiceStub::PlayVibratorEffectPb;
    baseFuncs_[PLAY_CUSTOM_VIBRATOR_EFFECT] = &MiscdeviceServiceStub::PlayCustomVibratorEffectPb;
    baseFuncs_[STOP_VIBRATOR_EFFECT] = &MiscdeviceServiceStub::StopVibratorEffectPb;
    baseFuncs_[SET_VIBRATOR_PARA] = &MiscdeviceServiceStub::SetVibratorParameterPb;
    baseFuncs_[GET_VIBRATOR_PARA] = &MiscdeviceServiceStub::GetVibratorParameterPb;
    baseFuncs_[GET_LIGHT_SUPPORT_ID] = &MiscdeviceServiceStub::GetLightSupportIdPb;
    baseFuncs_[IS_LIGHT_EFFECT_SUPPORT] = &MiscdeviceServiceStub::IsLightEffectSupportPb;
    baseFuncs_[LIGHT] = &MiscdeviceServiceStub::LightPb;
    baseFuncs_[PLAY_LIGHT_EFFECT] = &MiscdeviceServiceStub::PlayLightEffectPb;
    baseFuncs_[STOP_LIGHT_EFFECT] = &MiscdeviceServiceStub::StopLightEffectPb;
}

MiscdeviceServiceStub::~MiscdeviceServiceStub()
{
    HiLog::Info(LABEL, "%{public}s begin, xigou %{public}p", __func__, this);
    baseFuncs_.clear();
}

int32_t MiscdeviceServiceStub::IsAbilityAvailablePb(MessageParcel &data, MessageParcel &reply)
{
    MiscdeviceDeviceId groupId = static_cast<MiscdeviceDeviceId>(data.ReadUint32());
    reply.WriteBool(IsAbilityAvailable(groupId));
    return NO_ERROR;
}

int32_t MiscdeviceServiceStub::IsVibratorEffectAvailablePb(MessageParcel &data, MessageParcel &reply)
{
    int32_t vibratorId = data.ReadInt32();
    std::string effectType = data.ReadString();
    reply.WriteBool(IsVibratorEffectAvailable(vibratorId, effectType));
    return NO_ERROR;
}

int32_t MiscdeviceServiceStub::VibratePb(MessageParcel &data, MessageParcel &reply)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION)) {
        HiLog::Error(LABEL, "%{public}s permission denied", __func__);
        return ERR_PERMISSION_DENIED;
    }
    return Vibrate(data.ReadInt32(), data.ReadUint32());
}

int32_t MiscdeviceServiceStub::GetVibratorIdListPb(MessageParcel &data, MessageParcel &reply)
{
    std::vector<int32_t> idSet = GetVibratorIdList();
    reply.WriteUint32(static_cast<uint32_t>(idSet.size()));
    reply.WriteInt32Vector(idSet);
    return NO_ERROR;
}

int32_t MiscdeviceServiceStub::CancelVibratorPb(MessageParcel &data, MessageParcel &reply)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION)) {
        HiLog::Error(LABEL, "%{public}s permission denied", __func__);
        return ERR_PERMISSION_DENIED;
    }
    int32_t vibratorId = data.ReadInt32();
    return CancelVibrator(vibratorId);
}

int32_t MiscdeviceServiceStub::PlayVibratorEffectPb(MessageParcel &data, MessageParcel &reply)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION)) {
        HiLog::Error(LABEL, "%{public}s permission denied", __func__);
        return ERR_PERMISSION_DENIED;
    }
    int32_t vibratorId = data.ReadInt32();
    std::string effectType = data.ReadString();
    bool isLooping = data.ReadBool();
    return PlayVibratorEffect(vibratorId, effectType, isLooping);
}

int32_t MiscdeviceServiceStub::PlayCustomVibratorEffectPb(MessageParcel &data, MessageParcel &reply)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION)) {
        HiLog::Error(LABEL, "%{public}s permission denied", __func__);
        return ERR_PERMISSION_DENIED;
    }
    int32_t vibratorId = data.ReadInt32();
    std::vector<int32_t> timing;
    data.ReadInt32Vector(&timing);
    std::vector<int32_t> intensity;
    data.ReadInt32Vector(&intensity);
    int32_t periodCount = data.ReadInt32();
    return PlayCustomVibratorEffect(vibratorId, timing, intensity, periodCount);
}

int32_t MiscdeviceServiceStub::StopVibratorEffectPb(MessageParcel &data, MessageParcel &reply)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION)) {
        HiLog::Error(LABEL, "%{public}s permission denied", __func__);
        return ERR_PERMISSION_DENIED;
    }
    int32_t vibratorId = data.ReadInt32();
    std::string effectType = data.ReadString();
    return StopVibratorEffect(vibratorId, effectType);
}

int32_t MiscdeviceServiceStub::SetVibratorParameterPb(MessageParcel &data, MessageParcel &reply)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION)) {
        HiLog::Error(LABEL, "%{public}s permission denied", __func__);
        return ERR_PERMISSION_DENIED;
    }
    int32_t vibratorId = data.ReadInt32();
    std::string cmd = data.ReadString();
    return SetVibratorParameter(vibratorId, cmd);
}

int32_t MiscdeviceServiceStub::GetVibratorParameterPb(MessageParcel &data, MessageParcel &reply)
{
    int32_t vibratorId = data.ReadInt32();
    std::string cmd = data.ReadString();
    std::string ret = GetVibratorParameter(vibratorId, cmd);
    reply.WriteString(ret);
    return NO_ERROR;
}

int32_t MiscdeviceServiceStub::GetLightSupportIdPb(MessageParcel &data, MessageParcel &reply)
{
    std::vector<int32_t> idSet = GetLightSupportId();
    int32_t setCount = int32_t { idSet.size() };
    reply.WriteInt32(setCount);
    reply.WriteInt32Vector(idSet);
    return NO_ERROR;
}

int32_t MiscdeviceServiceStub::IsLightEffectSupportPb(MessageParcel &data, MessageParcel &reply)
{
    int32_t id = data.ReadInt32();
    std::string effect = data.ReadString();
    reply.WriteBool(IsLightEffectSupport(id, effect));
    return NO_ERROR;
}

int32_t MiscdeviceServiceStub::LightPb(MessageParcel &data, MessageParcel &reply)
{
    int32_t id = data.ReadInt32();
    uint32_t brightness = data.ReadUint64();
    uint32_t timeOn = data.ReadUint32();
    uint32_t timeOff = data.ReadUint32();
    return Light(id, brightness, timeOn, timeOff);
}

int32_t MiscdeviceServiceStub::PlayLightEffectPb(MessageParcel &data, MessageParcel &reply)
{
    int32_t id = data.ReadInt32();
    std::string type = data.ReadString();
    return PlayLightEffect(id, type);
}

int32_t MiscdeviceServiceStub::StopLightEffectPb(MessageParcel &data, MessageParcel &reply)
{
    int32_t id = data.ReadInt32();
    return StopLightEffect(id);
}

int32_t MiscdeviceServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                               MessageOption &option)
{
    HiLog::Debug(LABEL, "%{public}s begin, cmd : %{public}u", __func__, code);
    std::u16string descriptor = MiscdeviceServiceStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        HiLog::Error(LABEL, "%{public}s client and service descriptors are inconsistent", __func__);
        return OBJECT_NULL;
    }
    auto itFunc = baseFuncs_.find(code);
    if (itFunc != baseFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }
    HiLog::Debug(LABEL, "%{public}s no member function default process", __func__);
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}
}  // namespace Sensors
}  // namespace OHOS
