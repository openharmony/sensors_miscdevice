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
#include "compatible_connection.h"

#include <ctime>
#include <string>
#include <vector>

#include "sensors_errors.h"
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_SERVICE, "CompatibleConnection" };
std::vector<std::string> vibratorEffect_ = {"haptic.clock.timer"};
IVibratorHdiConnection::VibratorStopMode vibrateMode_;
constexpr uint32_t MAX_VIBRATOR_TIME = 1800000;
constexpr uint32_t MIN_VIBRATOR_TIME = 0;
}
uint32_t CompatibleConnection::duration_ = -1;
std::atomic_bool CompatibleConnection::isStop_ = false;

int32_t CompatibleConnection::ConnectHdi()
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    return ERR_OK;
}

int32_t CompatibleConnection::StartOnce(uint32_t duration)
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    if (duration > MAX_VIBRATOR_TIME || duration <= MIN_VIBRATOR_TIME) {
        HiLog::Error(LABEL, "%{public}s duration: %{public}d invalid", __func__, duration);
        return -1;
    }
    duration_ = duration;
    if (!vibrateThread_.joinable()) {
        std::thread senocdDataThread(CompatibleConnection::VibrateProcess);
        vibrateThread_ = std::move(senocdDataThread);
        isStop_ = false;
    }
    vibrateMode_ = VIBRATOR_STOP_MODE_TIME;
    return ERR_OK;
}

int32_t CompatibleConnection::Start(const char *effectType)
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    if (std::find(vibratorEffect_.begin(), vibratorEffect_.end(), effectType) == vibratorEffect_.end()) {
        HiLog::Error(LABEL, "%{public}s not support %{public}s type", __func__, effectType);
        return -1;
    }
    if (!vibrateThread_.joinable()) {
        std::thread senocdDataThread(CompatibleConnection::VibrateProcess);
        vibrateThread_ = std::move(senocdDataThread);
        isStop_ = false;
    }
    vibrateMode_ = VIBRATOR_STOP_MODE_PRESET;
    return ERR_OK;
}

int32_t CompatibleConnection::Stop(VibratorStopMode mode)
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    if (mode < 0 || mode >= VIBRATOR_STOP_MODE_INVALID) {
        HiLog::Error(LABEL, "%{public}s mode: %{public}d invalid", __func__, mode);
        return -1;
    }
    if (vibrateMode_ != mode) {
        HiLog::Error(LABEL, "%{public}s should start vibrate first", __func__);
        return -1;
    }
    if (vibrateThread_.joinable()) {
        HiLog::Info(LABEL, "%{public}s stop vibrate thread", __func__);
        isStop_ = true;
        vibrateThread_.join();
    }
    return ERR_OK;
}

int32_t CompatibleConnection::DestroyHdiConnection()
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    return ERR_OK;
}

void CompatibleConnection::VibrateProcess()
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    clock_t vibrateStartTime = clock();
    while (static_cast<uint32_t>(clock() - vibrateStartTime) < duration_) {
        if (isStop_) {
            HiLog::Info(LABEL, "%{public}s thread should stop", __func__);
            break;
        }
    }
    HiLog::Info(LABEL, "%{public}s end", __func__);
    return;
}
}  // namespace Sensors
}  // namespace OHOS
