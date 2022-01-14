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

#include "hdi_direct_connection.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"
#include "vibrator_type.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_SERVICE, "HdiDirectConnection" };
}

int32_t HdiDirectConnection::ConnectHdi()
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    vibratorInterface = NewVibratorInterfaceInstance();
    if (vibratorInterface == nullptr) {
        HiLog::Error(LABEL, "%{public}s failed, vibrator interface initialization failed", __func__);
        return ERR_INVALID_VALUE;
    }
    return ERR_OK;
}

int32_t HdiDirectConnection::StartOnce(uint32_t duration)
{
    int32_t ret = vibratorInterface->StartOnce(duration);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiDirectConnection::Start(const char *effectType)
{
    if (effectType == nullptr) {
        HiLog::Error(LABEL, "%{public}s effectType is null", __func__);
        return -1;
    }
    int32_t ret = vibratorInterface->Start(effectType);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiDirectConnection::Stop(VibratorStopMode mode)
{
    int32_t ret = vibratorInterface->Stop(static_cast<VibratorMode>(mode));
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiDirectConnection::DestroyHdiConnection()
{
    int32_t ret = FreeVibratorInterfaceInstance();
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    return ERR_OK;
}
}  // namespace Sensors
}  // namespace OHOS
