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

#include "vibration_priority_manager.h"

#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "VibrationPriorityManager" };
}  // namespace

VibrationPriorityManager::VibrationPriorityManager() {}

VibrationPriorityManager::~VibrationPriorityManager() {}

VibrateStatus VibrationPriorityManager::ShouldIgnoreVibrate(const VibrateInfo &vibrateInfo,
    std::shared_ptr<VibratorThread> vibratorThread)
{
    if (vibratorThread == nullptr) {
        MISC_HILOGD("There is no vibration, it can vibrate");
        return VIBRATION;
    }
    if (!IsCurrentVibrate(vibratorThread)) {
        MISC_HILOGD("There is no vibration at the moment, it can vibrate");
        return VIBRATION;
    }
    if (IsLoopVibrate(vibrateInfo)) {
        MISC_HILOGD("Can vibrate, loop priority is high");
        return VIBRATION;
    }
    return ShouldIgnoreVibrate(vibrateInfo, vibratorThread->GetCurrentVibrateInfo());
}

bool VibrationPriorityManager::IsCurrentVibrate(std::shared_ptr<VibratorThread> vibratorThread) const
{
    return ((vibratorThread != nullptr) && (vibratorThread->IsRunning()));
}

bool VibrationPriorityManager::IsLoopVibrate(const VibrateInfo &vibrateInfo) const
{
    return ((vibrateInfo.mode == "preset") && (vibrateInfo.count > 1));
}

VibrateStatus VibrationPriorityManager::ShouldIgnoreVibrate(const VibrateInfo &vibrateInfo,
    VibrateInfo currentVibrateInfo) const
{
    if (currentVibrateInfo.usage == USAGE_ALARM) {
        MISC_HILOGD("Vibration is ignored for alarm");
        return IGNORE_ALARM;
    }
    if (IsLoopVibrate(currentVibrateInfo)) {
        MISC_HILOGD("Vibration is ignored for repeat");
        return IGNORE_REPEAT;
    }
    if ((currentVibrateInfo.usage != vibrateInfo.usage) && (vibrateInfo.usage == USAGE_UNKNOWN)) {
        MISC_HILOGD("Vibration is ignored, unknown has a low priority");
        return IGNORE_UNKNOWN;
    }
    return VIBRATION;
}
}  // namespace Sensors
}  // namespace OHOS