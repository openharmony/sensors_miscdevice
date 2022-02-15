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
#include <thread>
#include "hdi_connection.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_SERVICE, "HdiConnection" };
constexpr int32_t GET_HDI_SERVICE_COUNT = 10;
constexpr uint32_t WAIT_MS = 100;
}

int32_t HdiConnection::ConnectHdi()
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    int32_t retry = 0;
    while (retry < GET_HDI_SERVICE_COUNT) {
        vibratorInterface_ = IVibratorInterface::Get();
        if (vibratorInterface_ != nullptr) {
            RegisterHdiDeathRecipient();
            return ERR_OK;
        }
        retry++;
        HiLog::Warn(LABEL, "%{public}s connect hdi service failed, retry : %{public}d", __func__, retry);
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
    }
    HiLog::Error(LABEL, "%{public}s failed, vibrator interface initialization failed", __func__);
    return ERR_INVALID_VALUE;
}

int32_t HdiConnection::StartOnce(uint32_t duration)
{
    int32_t ret = vibratorInterface_->StartOnce(duration);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::Start(const char *effectType)
{
    if (effectType == nullptr) {
        HiLog::Error(LABEL, "%{public}s effectType is null", __func__);
        return VIBRATOR_ON_ERR;
    }
    int32_t ret = vibratorInterface_->Start(effectType);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::Stop(VibratorStopMode mode)
{
    int32_t ret = vibratorInterface_->Stop(static_cast<OHOS::HDI::Vibrator::V1_0::HdfVibratorMode>(mode));
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::DestroyHdiConnection()
{
    UnregisterHdiDeathRecipient();
    return ERR_OK;
}

void HdiConnection::RegisterHdiDeathRecipient()
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (vibratorInterface_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s connect v1_0 hdi failed", __func__);
        return;
    }
    hdiDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<HdiConnection *>(this));
    if (hdiDeathObserver_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s hdiDeathObserver_ cannot be null", __func__);
        return;
    }
    vibratorInterface_->AsObject()->AddDeathRecipient(hdiDeathObserver_);
}

void HdiConnection::UnregisterHdiDeathRecipient()
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (vibratorInterface_ == nullptr || hdiDeathObserver_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s vibratorInterface_ or hdiDeathObserver_ is null", __func__);
        return;
    }
    vibratorInterface_->AsObject()->RemoveDeathRecipient(hdiDeathObserver_);
}

void HdiConnection::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    sptr<IRemoteObject> hdiService = object.promote();
    if (hdiService == nullptr || hdiDeathObserver_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s invalid remote object or hdiDeathObserver_ is null", __func__);
        return;
    }
    hdiService->RemoveDeathRecipient(hdiDeathObserver_);
    reconnect();
}

void HdiConnection::reconnect()
{
    int32_t ret = ConnectHdi();
    if (ret != ERR_OK) {
        HiLog::Error(LABEL, "%{public}s connect hdi fail", __func__);
    }
}
}  // namespace Sensors
}  // namespace OHOS
