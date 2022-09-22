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

#ifndef VIBRATOR_INFOS_H
#define VIBRATOR_INFOS_H

#include <string>
#include <unordered_map>

namespace OHOS {
namespace Sensors {
namespace {
std::unordered_map<std::string, int32_t> vibratorEffects = {
    {"haptic.clock.timer", 2000},
    {"haptic.default.effect", 804}
};
} // namespace

struct VibrateInfo {
    std::string mode;
    std::string packageName;
    int32_t pid = -1;
    int32_t uid = -1;
    int32_t usage = 0;
    int32_t duration = 0;
    std::string effect;
    int32_t count = 0;
};

enum VibrateUsage {
    USAGE_UNKNOWN = 0,
    USAGE_ALARM = 1,
    USAGE_RING = 2,
    USAGE_NOTIFICATION = 3,
    USAGE_COMMUNICATION = 4,
    USAGE_TOUCH = 5,
    USAGE_MEDIA = 6,
    USAGE_PHYSICAL_FEEDBACK = 7,
    USAGE_SIMULATE_REALITY = 8,
    USAGE_MAX = 9,
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // VIBRATOR_INFOS_H