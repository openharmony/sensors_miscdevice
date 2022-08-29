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
static std::unordered_map<std::string, int32_t> VibratorEffectMap = {
    {"haptic.clock.timer", 2000},
    {"haptic.default.effect", 804}
};

typedef struct {
    std::string mode;
    std::string packageName;
    int32_t pid;
    uint32_t usage;
    uint32_t duration;
    std::string effect;
    uint32_t count;
} VibrateInfo;
}  // namespace Sensors
}  // namespace OHOS
#endif  // VIBRATOR_INFOS_H