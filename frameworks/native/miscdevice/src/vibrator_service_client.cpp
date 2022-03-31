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

#include "vibrator_service_client.h"

#include <thread>
#include "death_recipient_template.h"
#include "dmd_report.h"
#include "iservice_registry.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_NATIVE, "VibratorServiceClient" };
constexpr int32_t GET_SERVICE_MAX_COUNT = 30;
constexpr uint32_t WAIT_MS = 200;
}  // namespace

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
            MISC_HILOGD("get service success, retry : %{public}d", retry);
            serviceDeathObserver_ =
                new (std::nothrow) DeathRecipientTemplate(*const_cast<VibratorServiceClient *>(this));
            if (serviceDeathObserver_ != nullptr) {
                miscdeviceProxy_->AsObject()->AddDeathRecipient(serviceDeathObserver_);
            }
            return ERR_OK;
        }
        MISC_HILOGW("get service failed, retry : %{public}d", retry);
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
        retry++;
    }
    DmdReport::ReportException(MISC_SERVICE_EXCEPTION, "InitServiceClient", MISC_NATIVE_GET_SERVICE_ERR);
    MISC_HILOGE("get service failed");
    return MISC_NATIVE_GET_SERVICE_ERR;
}

std::vector<int32_t> VibratorServiceClient::GetVibratorIdList()
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return {};
    }
    return miscdeviceProxy_->GetVibratorIdList();
}

bool VibratorServiceClient::IsVibratorEffectSupport(int32_t vibratorId, const std::string &effect)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return false;
    }
    return miscdeviceProxy_->IsVibratorEffectAvailable(vibratorId, effect);
}

int32_t VibratorServiceClient::Vibrate(int32_t vibratorId, uint32_t timeOut)
{
    MISC_HILOGD("Vibrate begin, timeOut : %{public}u", timeOut);
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    return miscdeviceProxy_->Vibrate(vibratorId, timeOut);
}

int32_t VibratorServiceClient::Vibrate(int32_t vibratorId, const std::string &effect, bool isLooping)
{
    MISC_HILOGD("Vibrate begin, effect : %{public}s", effect.c_str());
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    return miscdeviceProxy_->PlayVibratorEffect(vibratorId, effect, isLooping);
}

int32_t VibratorServiceClient::Vibrate(int32_t vibratorId, std::vector<int32_t> timing, std::vector<int32_t> intensity,
                                       int32_t periodCount)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    return miscdeviceProxy_->PlayCustomVibratorEffect(vibratorId, timing, intensity, periodCount);
}

int32_t VibratorServiceClient::Stop(int32_t vibratorId, const std::string &type)
{
    MISC_HILOGD("Stop begin, vibratorId : %{public}d, type : %{public}s", vibratorId, type.c_str());
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    if (type == "time") {
        ret = miscdeviceProxy_->CancelVibrator(vibratorId);
    } else {
        ret = miscdeviceProxy_->StopVibratorEffect(vibratorId, type);
    }
    return ret;
}

int32_t VibratorServiceClient::SetVibratorParameter(int32_t vibratorId, const std::string &cmd)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    return miscdeviceProxy_->SetVibratorParameter(vibratorId, cmd);
}

std::string VibratorServiceClient::GetVibratorParameter(int32_t vibratorId, const std::string &command)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return {};
    }
    return miscdeviceProxy_->GetVibratorParameter(vibratorId, command);
}

void VibratorServiceClient::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    CALL_LOG_ENTER;
    (void)object;
    miscdeviceProxy_ = nullptr;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return;
    }
}
}  // namespace Sensors
}  // namespace OHOS
