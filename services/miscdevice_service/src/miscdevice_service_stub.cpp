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

#include "miscdevice_service_stub.h"

#include <string>
#include <unistd.h>

#include "hisysevent.h"
#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "securec.h"

#include "permission_util.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "MiscdeviceServiceStub" };
const std::string VIBRATE_PERMISSION = "ohos.permission.VIBRATE";
}  // namespace

MiscdeviceServiceStub::MiscdeviceServiceStub()
{
    baseFuncs_[VIBRATE] = &MiscdeviceServiceStub::VibrateStub;
    baseFuncs_[PLAY_VIBRATOR_EFFECT] = &MiscdeviceServiceStub::PlayVibratorEffectStub;
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    baseFuncs_[PLAY_VIBRATOR_CUSTOM] = &MiscdeviceServiceStub::PlayVibratorCustomStub;
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    baseFuncs_[STOP_VIBRATOR_ALL] = &MiscdeviceServiceStub::StopVibratorAllStub;
    baseFuncs_[STOP_VIBRATOR_BY_MODE] = &MiscdeviceServiceStub::StopVibratorByModeStub;
    baseFuncs_[GET_LIGHT_LIST] = &MiscdeviceServiceStub::GetLightListStub;
    baseFuncs_[TURN_ON] = &MiscdeviceServiceStub::TurnOnStub;
    baseFuncs_[TURN_OFF] = &MiscdeviceServiceStub::TurnOffStub;
}

MiscdeviceServiceStub::~MiscdeviceServiceStub()
{
    baseFuncs_.clear();
}

int32_t MiscdeviceServiceStub::VibrateStub(MessageParcel &data, MessageParcel &reply)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "VibrateStub", "ERROR_CODE", ret);
        MISC_HILOGE("result: %{public}d", ret);
        return PERMISSION_DENIED;
    }
    int32_t vibratorId;
    int32_t duration;
    int32_t usage;
    if ((!data.ReadInt32(vibratorId)) || (!data.ReadInt32(duration)) || (!data.ReadInt32(usage))) {
        MISC_HILOGE("Parcel read failed");
        return ERROR;
    }
    return Vibrate(vibratorId, duration, usage);
}

int32_t MiscdeviceServiceStub::StopVibratorAllStub(MessageParcel &data, MessageParcel &reply)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "StopVibratorStub", "ERROR_CODE", ret);
        MISC_HILOGE("result:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    int32_t vibratorId;
    if (!data.ReadInt32(vibratorId)) {
        MISC_HILOGE("Parcel read failed");
        return ERROR;
    }
    return StopVibrator(vibratorId);
}

int32_t MiscdeviceServiceStub::PlayVibratorEffectStub(MessageParcel &data, MessageParcel &reply)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "PlayVibratorEffectStub", "ERROR_CODE", ret);
        MISC_HILOGE("result:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    int32_t vibratorId;
    std::string effect;
    int32_t count;
    int32_t usage;
    if ((!data.ReadInt32(vibratorId)) || (!data.ReadString(effect)) ||
        (!data.ReadInt32(count)) || (!data.ReadInt32(usage))) {
        MISC_HILOGE("Parcel read failed");
        return ERROR;
    }
    return PlayVibratorEffect(vibratorId, effect, count, usage);
}

int32_t MiscdeviceServiceStub::StopVibratorByModeStub(MessageParcel &data, MessageParcel &reply)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "StopVibratorByModeStub", "ERROR_CODE", ret);
        MISC_HILOGE("result:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    int32_t vibratorId;
    std::string mode;
    if ((!data.ReadInt32(vibratorId)) || (!data.ReadString(mode))) {
        MISC_HILOGE("Parcel read failed");
        return ERROR;
    }
    return StopVibrator(vibratorId, mode);
}

#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
int32_t MiscdeviceServiceStub::PlayVibratorCustomStub(MessageParcel &data, MessageParcel &reply)
{
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckVibratePermission(this->GetCallingTokenID(), VIBRATE_PERMISSION);
    if (ret != PERMISSION_GRANTED) {
        HiSysEventWrite(HiSysEvent::Domain::MISCDEVICE, "VIBRATOR_PERMISSIONS_EXCEPTION",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "PlayVibratorCustomStub", "ERROR_CODE", ret);
        MISC_HILOGE("CheckVibratePermission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    int32_t vibratorId;
    if (!data.ReadInt32(vibratorId)) {
        MISC_HILOGE("Parcel read vibratorId failed");
        return ERROR;
    }
    int32_t usage;
    if (!data.ReadInt32(usage)) {
        MISC_HILOGE("Parcel read usage failed");
        return ERROR;
    }
    RawFileDescriptor rawFd;
    if (!data.ReadInt64(rawFd.offset)) {
        MISC_HILOGE("Parcel read offset failed");
        return ERROR;
    }
    if (!data.ReadInt64(rawFd.length)) {
        MISC_HILOGE("Parcel read length failed");
        return ERROR;
    }
    rawFd.fd = data.ReadFileDescriptor();
    if (rawFd.fd < 0) {
        MISC_HILOGE("Parcel ReadFileDescriptor failed");
        return ERROR;
    }
    ret = PlayVibratorCustom(vibratorId, rawFd, usage);
    close(rawFd.fd);
    if (ret != ERR_OK) {
        MISC_HILOGE("PlayVibratorCustom failed, ret:%{public}d", ret);
        return ret;
    }
    return ret;
}
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM

int32_t MiscdeviceServiceStub::GetLightListStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    std::vector<LightInfo> lightInfos(GetLightList());
    size_t lightCount = lightInfos.size();
    MISC_HILOGE("lightCount:%{public}zu", lightCount);
    if (!reply.WriteUint32(lightCount)) {
        MISC_HILOGE("Parcel write failed");
        return WRITE_MSG_ERR;
    }
    for (size_t i = 0; i < lightCount; ++i) {
        if (!reply.WriteBuffer(&lightInfos[i], sizeof(LightInfo))) {
            MISC_HILOGE("WriteBuffer failed");
            return WRITE_MSG_ERR;
        }
    }
    return NO_ERROR;
}

int32_t MiscdeviceServiceStub::TurnOnStub(MessageParcel &data, MessageParcel &reply)
{
    int32_t lightId = data.ReadInt32();
    const unsigned char *info = data.ReadBuffer(sizeof(LightColor));
    CHKPR(info, ERROR);
    LightColor lightColor;
    if (memcpy_s(&lightColor, sizeof(LightColor), info, sizeof(LightColor)) != EOK) {
        MISC_HILOGE("memcpy_s lightColor failed");
        return ERROR;
    }

    const unsigned char *buf = data.ReadBuffer(sizeof(LightAnimation));
    CHKPR(buf, ERROR);
    LightAnimation lightAnimation;
    if (memcpy_s(&lightAnimation, sizeof(LightAnimation), buf, sizeof(LightAnimation)) != EOK) {
        MISC_HILOGE("memcpy_s lightAnimation failed");
        return ERROR;
    }
    return TurnOn(lightId, lightColor, lightAnimation);
}

int32_t MiscdeviceServiceStub::TurnOffStub(MessageParcel &data, MessageParcel &reply)
{
    int32_t lightId = data.ReadInt32();
    return TurnOff(lightId);
}

int32_t MiscdeviceServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                               MessageOption &option)
{
    MISC_HILOGD("remoterequest begin, cmd:%{public}u", code);
    std::u16string descriptor = MiscdeviceServiceStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        MISC_HILOGE("client and service descriptors are inconsistent");
        return OBJECT_NULL;
    }
    auto itFunc = baseFuncs_.find(code);
    if (itFunc != baseFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }
    MISC_HILOGD("remoterequest no member function default process");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}
}  // namespace Sensors
}  // namespace OHOS
