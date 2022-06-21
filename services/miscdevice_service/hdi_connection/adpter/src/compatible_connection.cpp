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

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "CompatibleConnection" };
std::vector<std::string> vibratorEffect_ = {"haptic.clock.timer"};
IVibratorHdiConnection::VibratorStopMode vibrateMode_;
constexpr uint32_t MAX_VIBRATOR_TIME = 1800000;
constexpr uint32_t MIN_VIBRATOR_TIME = 0;
}
uint32_t CompatibleConnection::duration_ = -1;
std::atomic_bool CompatibleConnection::isStop_ = false;

int32_t CompatibleConnection::ConnectHdi()
{
    CALL_LOG_ENTER;
    return ERR_OK;
}

int32_t CompatibleConnection::StartOnce(uint32_t duration)
{
    CALL_LOG_ENTER;
    if (duration > MAX_VIBRATOR_TIME || duration <= MIN_VIBRATOR_TIME) {
        MISC_HILOGE("duration: %{public}d invalid", duration);
        return VIBRATOR_ON_ERR;
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

int32_t CompatibleConnection::Start(const std::string &effectType)
{
    CALL_LOG_ENTER;
    if (std::find(vibratorEffect_.begin(), vibratorEffect_.end(), effectType) == vibratorEffect_.end()) {
        MISC_HILOGE("Not support %{public}s type", effectType.c_str());
        return VIBRATOR_ON_ERR;
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
    CALL_LOG_ENTER;
    if (mode < 0 || mode >= VIBRATOR_STOP_MODE_INVALID) {
        MISC_HILOGE("mode: %{public}d invalid", mode);
        return VIBRATOR_OFF_ERR;
    }
    if (vibrateMode_ != mode) {
        MISC_HILOGE("should start vibrate first");
        return VIBRATOR_OFF_ERR;
    }
    if (vibrateThread_.joinable()) {
        MISC_HILOGI("stop vibrate thread");
        isStop_ = true;
        vibrateThread_.join();
    }
    return ERR_OK;
}

int32_t CompatibleConnection::DestroyHdiConnection()
{
    CALL_LOG_ENTER;
    return ERR_OK;
}

void CompatibleConnection::VibrateProcess()
{
    CALL_LOG_ENTER;
    clock_t vibrateStartTime = clock();
    while (static_cast<uint32_t>(clock() - vibrateStartTime) < duration_) {
        if (isStop_) {
            MISC_HILOGI("thread should stop");
            break;
        }
    }
    return;
}
}  // namespace Sensors
}  // namespace OHOS
