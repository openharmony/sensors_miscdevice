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

#ifndef VIBRATION_PRIORITY_MANAGER_H
#define VIBRATION_PRIORITY_MANAGER_H
#include <cstring>
#include <memory>
#include <stdint.h>
#include <vector>

#include "miscdevice_json_parser.h"
#include "singleton.h"
#include "vibrator_infos.h"
#include "vibrator_thread.h"
namespace OHOS {
namespace Sensors {

typedef enum {
    VIBRATION = 0,
    IGNORE_FOR_BACKGROUND = 1,
    IGNORE_FOR_LOW_POWER = 2,
    IGNORE_FOR_GLOBAL_SETTINGS = 3,
    IGNORE_FOR_RINGTONE = 4,
    IGNORE_FOR_REPEAT = 5,
    IGNORE_FOR_ALARM = 6,
    IGNORE_FOR_UNKNOWN = 7,
} VibrateStatus;

typedef enum {
    USAGE_UNKNOWN = 0,            /**< Vibration is used for unknown, lowest priority */
    USAGE_ALARM = 1,              /**< Vibration is used for alarm */
    USAGE_RING = 2,               /**< Vibration is used for ring */
    USAGE_NOTIFICATION = 3,       /**< Vibration is used for notification */
    USAGE_COMMUNICATION = 4,      /**< Vibration is used for communication */
    USAGE_TOUCH = 5,              /**< Vibration is used for touch */
    USAGE_MEDIA = 6,              /**< Vibration is used for media */
    USAGE_PHYSICAL_FEEDBACK = 7,  /**< Vibration is used for physical feedback */
    USAGE_SIMULATE_REALITY = 8,   /**< Vibration is used for simulate reality */
    USAGE_MAX = 9,
} VibrateUsage;

typedef struct {
    std::vector<std::string> byPassUsages;
    std::vector<std::string> byPassPkgs;
    std::vector<std::string> filterUsages;
    std::vector<std::string> filterPkgs;
} PriorityItem;

typedef struct {
    PriorityItem calling;
    PriorityItem camera;
} SpecialForeground;

typedef struct {
    std::vector<std::string> privilegePkgs;
    PriorityItem globalSettings;
    PriorityItem lowPowerMode;
    SpecialForeground specialForeground;
} PriorityConfig;

class VibrationPriorityManager : public Singleton<VibrationPriorityManager> {
public:
    VibrationPriorityManager();
    VibrateStatus ShouldIgnoreVibrate(VibrateInfo vibrateInfo, std::shared_ptr<VibratorThread> vibratorThread);

private:
    int32_t LoadPriorityConfig(const std::string &configPath);
    int32_t ParserPriorityItem(cJSON *json, const std::string& key, PriorityItem& item);
    PriorityConfig PriorityConfig_;
    std::unique_ptr<MiscdeviceJsonParser> jsonParser_;
    DISALLOW_COPY_AND_MOVE(VibrationPriorityManager);
};

}  // namespace Sensors
}  // namespace OHOS
#endif  // VIBRATION_PRIORITY_MANAGER_H