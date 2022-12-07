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

#ifndef CUSTOM_VIBRATION_MATCHER_H
#define CUSTOM_VIBRATION_MATCHER_H

#include <string>
#include <vector>

#include "vibrator_infos.h"

namespace OHOS {
namespace Sensors {
class CustomVibrationMatcher {
public:
    explicit CustomVibrationMatcher(const std::vector<VibrateEvent>& vibrateSequence);
    ~CustomVibrationMatcher() = default;
    std::vector<int32_t> GetVibrateSequence() const { return convertSequence_; }
    static bool ParameterCheck(const std::vector<VibrateEvent>& vibrateSequence);
private:
    void ProcessContinuousEvent(const VibrateEvent& event);
    void ProcessTransientEvent(const VibrateEvent& event);
    static void Normalize(std::vector<std::vector<float>>& matrix);
    std::vector<int32_t> convertSequence_;
};
}  // namespace Sensors
}  // namespace OHOS
#endif // CUSTOM_VIBRATION_MATCHER_H
