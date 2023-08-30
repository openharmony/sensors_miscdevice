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

#include "vibrator_service_client.h"

#include <thread>

#include "hisysevent.h"
#include "hitrace_meter.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "death_recipient_template.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "VibratorServiceClient" };
constexpr int32_t GET_SERVICE_MAX_COUNT = 30;
constexpr uint32_t WAIT_MS = 200;
}  // namespace

VibratorServiceClient::~VibratorServiceClient()
{
    if (miscdeviceProxy_ != nullptr && serviceDeathObserver_ != nullptr) {
        auto remoteObject = miscdeviceProxy_->AsObject();
        if (remoteObject != nullptr) {
            remoteObject->RemoveDeathRecipient(serviceDeathObserver_);
        }
    }
}

int32_t VibratorServiceClient::InitServiceClient()
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    if (miscdeviceProxy_ != nullptr) {
        MISC_HILOGW("miscdeviceProxy_ already init");
        return ERR_OK;
    }
    auto sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sm == nullptr) {
        MISC_HILOGE("sm cannot be null");
        return MISC_NATIVE_SAM_ERR;
    }
    int32_t retry = 0;
    while (retry < GET_SERVICE_MAX_COUNT) {
        miscdeviceProxy_ = iface_cast<IMiscdeviceService>(sm->GetSystemAbility(MISCDEVICE_SERVICE_ABILITY_ID));
        if (miscdeviceProxy_ != nullptr) {
            MISC_HILOGD("Get service success, retry:%{public}d", retry);
            serviceDeathObserver_ =
                new (std::nothrow) DeathRecipientTemplate(*const_cast<VibratorServiceClient *>(this));
            CHKPR(serviceDeathObserver_, MISC_NATIVE_GET_SERVICE_ERR);
            auto remoteObject = miscdeviceProxy_->AsObject();
            CHKPR(remoteObject, MISC_NATIVE_GET_SERVICE_ERR);
            remoteObject->AddDeathRecipient(serviceDeathObserver_);
            return ERR_OK;
        }
        MISC_HILOGW("Get service failed, retry:%{public}d", retry);
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
        retry++;
    }
    HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_EXCEPTION",
        HiSysEvent::EventType::FAULT, "PKG_NAME", "InitServiceClient", "ERROR_CODE", MISC_NATIVE_GET_SERVICE_ERR);
    MISC_HILOGE("Get service failed");
    return MISC_NATIVE_GET_SERVICE_ERR;
}

int32_t VibratorServiceClient::Vibrate(int32_t vibratorId, int32_t timeOut, int32_t usage)
{
    MISC_HILOGD("Vibrate begin, timeOut:%{public}u", timeOut);
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    CHKPR(miscdeviceProxy_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "VibrateTime");
    ret = miscdeviceProxy_->Vibrate(vibratorId, timeOut, usage);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != ERR_OK) {
        MISC_HILOGE("Vibrate time failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t VibratorServiceClient::Vibrate(int32_t vibratorId, const std::string &effect,
    int32_t loopCount, int32_t usage)
{
    MISC_HILOGD("Vibrate begin, effect:%{public}s", effect.c_str());
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    CHKPR(miscdeviceProxy_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "VibrateEffect");
    ret = miscdeviceProxy_->PlayVibratorEffect(vibratorId, effect, loopCount, usage);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != ERR_OK) {
        MISC_HILOGE("Vibrate effect failed, ret:%{public}d", ret);
    }
    return ret;
}

#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
int32_t VibratorServiceClient::PlayVibratorCustom(int32_t vibratorId, const RawFileDescriptor &rawFd, int32_t usage)
{
    MISC_HILOGD("PlayVibratorCustom begin, fd:%{public}d, offset:%{public}lld, length:%{public}lld",
        rawFd.fd, static_cast<long long>(rawFd.offset), static_cast<long long>(rawFd.length));
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    CHKPR(miscdeviceProxy_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "PlayVibratorCustom");
    ret = miscdeviceProxy_->PlayVibratorCustom(vibratorId, rawFd, usage);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != ERR_OK) {
        MISC_HILOGE("PlayVibratorCustom failed, ret:%{public}d", ret);
    }
    return ret;
}
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM

int32_t VibratorServiceClient::StopVibrator(int32_t vibratorId, const std::string &mode)
{
    MISC_HILOGD("StopVibrator begin, vibratorId:%{public}d, mode:%{public}s", vibratorId, mode.c_str());
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    CHKPR(miscdeviceProxy_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "StopVibratorByMode");
    ret = miscdeviceProxy_->StopVibrator(vibratorId, mode);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != ERR_OK) {
        MISC_HILOGE("StopVibrator by mode failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t VibratorServiceClient::StopVibrator(int32_t vibratorId)
{
    MISC_HILOGD("StopVibrator begin, vibratorId:%{public}d", vibratorId);
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    CHKPR(miscdeviceProxy_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "StopVibratorAll");
    ret = miscdeviceProxy_->StopVibrator(vibratorId);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != ERR_OK) {
        MISC_HILOGE("StopVibrator failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t VibratorServiceClient::IsSupportEffect(const std::string &effect, bool &state)
{
    MISC_HILOGD("IsSupportEffect begin, effect:%{public}s", effect.c_str());
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    CHKPR(miscdeviceProxy_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "VibrateEffect");
    ret = miscdeviceProxy_->IsSupportEffect(effect, state);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != ERR_OK) {
        MISC_HILOGE("Query effect support failed, ret:%{public}d", ret);
    }
    return ret;
}

void VibratorServiceClient::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    CALL_LOG_ENTER;
    (void)object;
    miscdeviceProxy_ = nullptr;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return;
    }
}
}  // namespace Sensors
}  // namespace OHOS
