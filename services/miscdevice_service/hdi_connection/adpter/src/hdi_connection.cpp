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

#include "hdi_connection.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_SERVICE, "HdiConnection" };
}

int32_t HdiConnection::ConnectHdi()
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    vibratorInterface_ = IVibratorInterface::Get();
    if (vibratorInterface_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s failed, vibrator interface initialization failed", __func__);
        return ERR_INVALID_VALUE;
    }
    return ERR_OK;
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
    int32_t ret = vibratorInterface_->Stop(static_cast<vibrator::v1_0::HdfVibratorMode>(mode));
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::DestroyHdiConnection()
{
    return ERR_OK;
}
}  // namespace Sensors
}  // namespace OHOS
