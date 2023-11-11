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
#include "vibrator_hdi_connection.h"

#include "hitrace_meter.h"

#include "compatible_connection.h"
#include "hdi_connection.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "VibratorHdiConnection" };
} // namespace

int32_t VibratorHdiConnection::ConnectHdi()
{
    iVibratorHdiConnection_ = std::make_unique<HdiConnection>();
    int32_t ret = iVibratorHdiConnection_->ConnectHdi();
    if (ret != ERR_OK) {
        MISC_HILOGE("Hdi direct failed");
        iVibratorHdiConnection_ = std::make_unique<CompatibleConnection>();
        ret = iVibratorHdiConnection_->ConnectHdi();
    }
    if (ret != ERR_OK) {
        MISC_HILOGE("Hdi connection failed");
        return VIBRATOR_HDF_CONNECT_ERR;
    }
    return ERR_OK;
}

int32_t VibratorHdiConnection::StartOnce(uint32_t duration)
{
    CHKPR(iVibratorHdiConnection_, VIBRATOR_HDF_CONNECT_ERR);
    StartTrace(HITRACE_TAG_SENSORS, "StartOnce");
    int32_t ret = iVibratorHdiConnection_->StartOnce(duration);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != 0) {
        MISC_HILOGE("StartOnce failed");
        return VIBRATOR_ON_ERR;
    }
    return ERR_OK;
}

int32_t VibratorHdiConnection::Start(const std::string &effectType)
{
    CHKPR(iVibratorHdiConnection_, VIBRATOR_HDF_CONNECT_ERR);
    StartTrace(HITRACE_TAG_SENSORS, "Start");
    int32_t ret = iVibratorHdiConnection_->Start(effectType);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != 0) {
        MISC_HILOGE("Start failed");
        return VIBRATOR_ON_ERR;
    }
    return ERR_OK;
}

#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
int32_t VibratorHdiConnection::EnableCompositeEffect(const HdfCompositeEffect &hdfCompositeEffect)
{
    CHKPR(iVibratorHdiConnection_, VIBRATOR_HDF_CONNECT_ERR);
    StartTrace(HITRACE_TAG_SENSORS, "EnableCompositeEffect");
    int32_t ret = iVibratorHdiConnection_->EnableCompositeEffect(hdfCompositeEffect);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != 0) {
        MISC_HILOGE("EnableCompositeEffect failed");
        return VIBRATOR_ON_ERR;
    }
    return ERR_OK;
}

bool VibratorHdiConnection::IsVibratorRunning()
{
    CHKPR(iVibratorHdiConnection_, VIBRATOR_HDF_CONNECT_ERR);
    return iVibratorHdiConnection_->IsVibratorRunning();
}
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM

std::optional<HdfEffectInfo> VibratorHdiConnection::GetEffectInfo(const std::string &effect)
{
    if (iVibratorHdiConnection_ == nullptr) {
        MISC_HILOGE("Connect hdi failed");
        return std::nullopt;
    }
    StartTrace(HITRACE_TAG_SENSORS, "GetEffectInfo");
    std::optional<HdfEffectInfo> ret = iVibratorHdiConnection_->GetEffectInfo(effect);
    FinishTrace(HITRACE_TAG_SENSORS);
    return ret;
}

int32_t VibratorHdiConnection::Stop(HdfVibratorMode mode)
{
    CHKPR(iVibratorHdiConnection_, VIBRATOR_HDF_CONNECT_ERR);
    StartTrace(HITRACE_TAG_SENSORS, "Stop");
    int32_t ret = iVibratorHdiConnection_->Stop(mode);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != 0) {
        MISC_HILOGE("Stop failed");
        return VIBRATOR_OFF_ERR;
    }
    return ERR_OK;
}

int32_t VibratorHdiConnection::GetDelayTime(int32_t mode, int32_t &delayTime)
{
    CHKPR(iVibratorHdiConnection_, VIBRATOR_HDF_CONNECT_ERR);
    return iVibratorHdiConnection_->GetDelayTime(mode, delayTime);
}

int32_t VibratorHdiConnection::GetVibratorCapacity(VibratorCapacity &capacity)
{
    CHKPR(iVibratorHdiConnection_, VIBRATOR_HDF_CONNECT_ERR);
    return iVibratorHdiConnection_->GetVibratorCapacity(capacity);
}
    
int32_t VibratorHdiConnection::PlayPattern(const VibratePattern &pattern)
{
    CHKPR(iVibratorHdiConnection_, VIBRATOR_HDF_CONNECT_ERR);
    return iVibratorHdiConnection_->PlayPattern(pattern);
}

int32_t VibratorHdiConnection::DestroyHdiConnection()
{
    CHKPR(iVibratorHdiConnection_, VIBRATOR_HDF_CONNECT_ERR);
    int32_t ret = iVibratorHdiConnection_->DestroyHdiConnection();
    if (ret != 0) {
        MISC_HILOGE("DestroyHdiConnection failed");
        return VIBRATOR_HDF_CONNECT_ERR;
    }
    return ERR_OK;
}
}  // namespace Sensors
}  // namespace OHOS
