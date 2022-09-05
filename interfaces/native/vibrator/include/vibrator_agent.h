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

/**
 * @addtogroup vibrator
 * @{
 *
 * @brief Provides functions for managing vibrators.
 * @since 6
 */

/**
 * @file vibrator_agent.h
 *
 * @brief Declares the functions for starting or stopping a vibrator.
 * @since 6
 */
#ifndef VIBRATOR_AGENT_H
#include <stdint.h>
#include "vibrator_agent_type.h"
#define VIBRATOR_AGENT_H

#ifdef __cplusplus
extern "C" {
#endif

namespace OHOS {
namespace Sensors {
/**
 * @brief Indicates the mode of stopping a one-shot vibration effect.
 *
 * @since 6
 */
const char *VIBRATOR_STOP_MODE_TIME = "time";

/**
 * @brief Indicates the mode of stopping a preset vibration effect.
 *
 * @since 6
 */
const char *VIBRATOR_STOP_MODE_PRESET = "preset";

/**
 * @brief Controls this vibrator to perform a vibration with a preset vibration effect.
 *
 * @param effectId Indicates the preset vibration effect, which is described in {@link vibrator_agent_type.h}, for
 * example:
 * {@link VIBRATOR_TYPE_CLOCK_TIMER}: Describes the vibration effect of the vibrator when a user adjusts the timer.
 * @return Returns <b>0</b> if the vibrator vibrates as expected; returns <b>-1</b> otherwise, for example, the preset
 * vibration effect is not supported.
 *
 * @since 6
 */
int32_t StartVibrator(const char *effectId);

/**
 * @brief Controls this vibrator to perform a one-shot vibration at a given duration.
 *
 * @param duration Indicates the duration that the one-shot vibration lasts, in milliseconds.
 * @return Returns <b>0</b> if the vibrator vibrates as expected; returns <b>-1</b> otherwise, for example, the given
 * duration for the one-shot vibration is invalid.
 *
 * @since 6
 */
int32_t StartVibratorOnce(int32_t duration);

/**
 * @brief Sets the number of cycles for vibration.
 * @param count Indicates the number of cycles for vibration.
 * @since 9
 */
bool SetLoopCount(int32_t count);

/**
 * @brief Stops the vibration of this vibrator.
 *
 * @param mode Indicates the mode of the vibration to stop. The values can be <b>time</b> and <b>preset</b>,
 * respectively representing a one-shot vibration effect and a preset vibration effect.
 * @return Returns <b>0</b> if the vibration is stopped as expected; returns <b>-1</b> otherwise.
 * @since 6
 */
int32_t StopVibrator(const char *mode);

/**
 * @brief Set the usage of vibration.
 *
 * @param usage Indicates the vibration usage, which is described in {@link vibrator_agent_type.h},for
 * example:
 * {@link USAGE_ALARM}: Describes the vibration is used for alarm.
 *
 * @since 9
 */
bool SetUsage(int32_t usage);
} // namespace Sensors
} // namespace OHOS
#ifdef __cplusplus
};
#endif
/** @} */
#endif // endif VIBRATOR_AGENT_H