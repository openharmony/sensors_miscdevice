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

#include "light_service_client.h"

#include <thread>
#include "death_recipient_template.h"
#include "hisysevent.h"
#include "iservice_registry.h"
#include "sensors_errors.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "LightServiceClient" };
constexpr int32_t GET_SERVICE_MAX_COUNT = 30;
constexpr uint32_t WAIT_MS = 200;
}  // namespace

int32_t LightServiceClient::InitServiceClient()
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    if (miscdeviceProxy_ != nullptr) {
        MISC_HILOGD("miscdeviceProxy_ already init");
        return ERR_OK;
    }
    auto sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHKPR(sm, MISC_NATIVE_SAM_ERR);
    int32_t retry = 0;
    while (retry < GET_SERVICE_MAX_COUNT) {
        miscdeviceProxy_ = iface_cast<IMiscdeviceService>(sm->GetSystemAbility(MISCDEVICE_SERVICE_ABILITY_ID));
        if (miscdeviceProxy_ != nullptr) {
            MISC_HILOGD("miscdeviceProxy_ get service success, retry : %{public}d", retry);
            serviceDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<LightServiceClient *>(this));
            if (serviceDeathObserver_ != nullptr) {
                miscdeviceProxy_->AsObject()->AddDeathRecipient(serviceDeathObserver_);
            }
            return ERR_OK;
        }
        MISC_HILOGW("get service failed, retry : %{public}d", retry);
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
        retry++;
    }
    HiSysEvent::Write(HiviewDFX::HiSysEvent::Domain::MISCDEVICE, "MISC_SERVICE_EXCEPTION",
        HiSysEvent::EventType::FAULT, "FUNC_NAME", "InitServiceClient", "ERROR_CODE", MISC_NATIVE_GET_SERVICE_ERR);
    MISC_HILOGE("get service failed");
    return MISC_NATIVE_GET_SERVICE_ERR;
}

std::vector<int32_t> LightServiceClient::GetLightIdList()
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return {};
    }
    return miscdeviceProxy_->GetLightSupportId();
}  // namespace Sensors

bool LightServiceClient::IsLightEffectSupport(int32_t lightId, const std::string &effectId)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return false;
    }
    return miscdeviceProxy_->IsLightEffectSupport(lightId, effectId);
}

int32_t LightServiceClient::Light(int32_t lightId, uint64_t brightness, uint32_t timeOn, uint32_t timeOff)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    return miscdeviceProxy_->Light(lightId, brightness, timeOn, timeOff);
}

int32_t LightServiceClient::PlayLightEffect(int32_t lightId, const std::string &type)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    return miscdeviceProxy_->PlayLightEffect(lightId, type);
}

int32_t LightServiceClient::StopLightEffect(int32_t lightId)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return MISC_NATIVE_GET_SERVICE_ERR;
    }
    return miscdeviceProxy_->StopLightEffect(lightId);
}

void LightServiceClient::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    CALL_LOG_ENTER;
    (void)object;
    miscdeviceProxy_ = nullptr;
    auto ret = InitServiceClient();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return;
    }
}
}  // namespace Sensors
}  // namespace OHOS
