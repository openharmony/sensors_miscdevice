/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "vibrator_thread.h"

#include "sensors_errors.h"
#include "vibrator_hdi_connection.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "VibratorThread" };
}  // namespace

std::mutex VibratorThread::conditionVarMutex_;
std::condition_variable VibratorThread::conditionVar_;

bool VibratorThread::Run()
{
    if (currentVibration_.mode == "time") {
        std::unique_lock<std::mutex> lck(conditionVarMutex_);
        int32_t ret = VibratorHdiConnection::GetInstance().StartOnce(currentVibration_.duration);
        if (ret != ERR_OK) {
            MISC_HILOGE("StartOnce fail, duration: %{public}d, pid: %{public}d",
                currentVibration_.duration, currentVibration_.pid);
            return false;
        }
        conditionVar_.wait_for(lck, std::chrono::milliseconds(currentVibration_.duration));
        if (ready_) {
            MISC_HILOGD("stop vibration");
        }
        VibratorHdiConnection::GetInstance().Stop(IVibratorHdiConnection::VIBRATOR_STOP_MODE_TIME);
    } else if (currentVibration_.mode == "preset") {
        for (uint32_t i = 0; i < currentVibration_.count; ++i) {
            std::unique_lock<std::mutex> lck(conditionVarMutex_);
            std::string effect = currentVibration_.effect;
            int32_t ret = VibratorHdiConnection::GetInstance().Start(effect);
            if (ret != ERR_OK) {
                MISC_HILOGE("vibrate effect %{public}s failed, pid: %{public}d", effect.c_str(), currentVibration_.pid);
                return false;
            }
            conditionVar_.wait_for(lck, std::chrono::milliseconds(currentVibration_.duration));
            if (ready_) {
                MISC_HILOGD("stop vibration");
            }
            VibratorHdiConnection::GetInstance().Stop(IVibratorHdiConnection::VIBRATOR_STOP_MODE_PRESET);
        }
    }
    return false;
}

void VibratorThread::UpdateVibratorEffect(VibrateInfo info)
{
    currentVibration_ = info;
}

VibrateInfo VibratorThread::GetCurrentVibrateInfo() const
{
    return currentVibration_;
}

void VibratorThread::SetReadyStatus(bool status)
{
    ready_ = status;
}
}  // namespace Sensors
}  // namespace OHOS
