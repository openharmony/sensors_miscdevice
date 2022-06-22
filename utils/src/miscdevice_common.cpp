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

#include "miscdevice_common.h"

#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "MiscdeviceCommon" };
constexpr int32_t MIN_VIBRATOR_COUNT = 0;
constexpr int32_t MAX_VIBRATOR_COUNT = 100;
constexpr int32_t MIN_VIBRATOR_INTENSITY = 0;
constexpr int32_t MAX_VIBRATOR_INTENSITY = 255;
constexpr int32_t HALF_AN_HOUR = 1800000;  // ms
}  // namespace

bool MiscdeviceCommon::CheckCustomVibratorEffect(const std::vector<int32_t> &timing,
                                                 const std::vector<int32_t> &intensity, int32_t periodCount)
{
    if ((periodCount < MIN_VIBRATOR_COUNT) || (periodCount > MAX_VIBRATOR_COUNT)) {
        MISC_HILOGE("input param invalid is failed");
        return false;
    }
    if (timing.size() != intensity.size()) {
        MISC_HILOGE("timing size invalid is failed");
        return false;
    }
    int32_t totalTime = 0;
    for (uint32_t i = 0; i < timing.size(); i = i + 2) {
        totalTime += timing[i];
    }
    if (totalTime > HALF_AN_HOUR) {
        MISC_HILOGE("totalTime invalid is failed");
        return false;
    }
    for (uint32_t i = 0; i < intensity.size(); i++) {
        if ((intensity[i] < MIN_VIBRATOR_INTENSITY) || (intensity[i] > MAX_VIBRATOR_INTENSITY)) {
            return false;
        }
    }
    return true;
}
}  // namespace Sensors
}  // namespace OHOS
