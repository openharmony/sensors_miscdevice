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

#ifndef VIBRATOR_AGENT_TYPE_H
#define VIBRATOR_AGENT_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Describes the vibration effect of the vibrator when a user adjusts the timer.
 *
 * @since 1
 */
const char *VIBRATOR_TYPE_CLOCK_TIMER = "haptic.clock.timer";

/**
 * @brief Enumerates vibration usages.
 *
 * @since 9
 */
typedef enum VibratorUsage {
    USAGE_UNKNOWN = 0,            /**< Vibration is used for unknown, lowest priority */
    USAGE_ALARM = 1,              /**< Vibration is used for alarm */
    USAGE_RING = 2,               /**< Vibration is used for ring */
    USAGE_NOTIFICATION = 3,       /**< Vibration is used for notification */
    USAGE_COMMUNICATION = 4,      /**< Vibration is used for communication */
    USAGE_TOUCH = 5,              /**< Vibration is used for touch */
    USAGE_MEDIA = 6,              /**< Vibration is used for media */
    USAGE_PHYSICAL_FEEDBACK = 7,  /**< Vibration is used for physical feedback */
    USAGE_SIMULATE_REALITY = 8,   /**< Vibration is used for simulate reality */
    USAGE_MAX = 9
} VibratorUsage;

/** @} */
#ifdef __cplusplus
};
#endif

#endif  // endif VIBRATOR_AGENT_TYPE_H