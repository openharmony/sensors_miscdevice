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
#include "vibrator_hdi_connection.h"

#include "compatible_connection.h"
#include "hdi_direct_connection.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_SERVICE, "VibratorHdiConnection" };
}

int32_t VibratorHdiConnection::ConnectHdi()
{
    iVibratorHdiConnection_ = std::make_unique<HdiDirectConnection>();
    int32_t ret = iVibratorHdiConnection_->ConnectHdi();
    if (ret != 0) {
        HiLog::Error(LABEL, "%{public}s hdi direct failed", __func__);
        iVibratorHdiConnection_ = std::make_unique<CompatibleConnection>();
        ret = iVibratorHdiConnection_->ConnectHdi();
    }
    if (ret != 0) {
        HiLog::Error(LABEL, "%{public}s hdi connection failed", __func__);
        return VIBRATOR_HDF_CONNECT_ERR;
    }
    return ERR_OK;
}

int32_t VibratorHdiConnection::StartOnce(uint32_t duration)
{
    int32_t ret = iVibratorHdiConnection_->StartOnce(duration);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return VIBRATOR_ON_ERR;
    }
    return ERR_OK;
}

int32_t VibratorHdiConnection::Start(const char *effectType)
{
    int32_t ret = iVibratorHdiConnection_->Start(effectType);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return VIBRATOR_ON_ERR;
    }
    return ERR_OK;
}

int32_t VibratorHdiConnection::Stop(VibratorStopMode mode)
{
    int32_t ret = iVibratorHdiConnection_->Stop(mode);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return VIBRATOR_OFF_ERR;
    }
    return ERR_OK;
}

int32_t VibratorHdiConnection::DestroyHdiConnection()
{
    int32_t ret = iVibratorHdiConnection_->DestroyHdiConnection();
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return VIBRATOR_HDF_CONNECT_ERR;
    }
    return ERR_OK;
}
}  // namespace Sensors
}  // namespace OHOS
