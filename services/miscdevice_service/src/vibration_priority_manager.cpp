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
const std::string CONFIG_PATH = "/vendor/etc/vibrator/vibration_policy_config.json";
}  // namespace

VibrationPriorityManager::VibrationPriorityManager()
{
    CALL_LOG_ENTER;
    PriorityConfig config;
    int32_t ret = LoadPriorityConfig(CONFIG_PATH, config);
    if (ret != SUCCESS) {
        MISC_HILOGE("Load priority config fail");
        return;
    }
    CopyPriorityConfig(config);
}

VibrationPriorityManager::~VibrationPriorityManager() {}

void VibrationPriorityManager::CopyPriorityConfig(PriorityConfig config)
{
    priorityConfig_.privilegePkgs = config.privilegePkgs;
    priorityConfig_.globalSettings.byPassUsages = config.globalSettings.byPassUsages;
    priorityConfig_.globalSettings.byPassPkgs = config.globalSettings.byPassPkgs;
    priorityConfig_.globalSettings.filterUsages = config.globalSettings.filterUsages;
    priorityConfig_.globalSettings.filterPkgs = config.globalSettings.filterPkgs;

    priorityConfig_.lowPowerMode.byPassUsages = config.lowPowerMode.byPassUsages;
    priorityConfig_.lowPowerMode.byPassPkgs = config.lowPowerMode.byPassPkgs;
    priorityConfig_.lowPowerMode.filterUsages = config.lowPowerMode.filterUsages;
    priorityConfig_.lowPowerMode.filterPkgs = config.lowPowerMode.filterPkgs;

    priorityConfig_.specialForeground.calling.byPassUsages = config.specialForeground.calling.byPassUsages;
    priorityConfig_.specialForeground.calling.byPassPkgs = config.specialForeground.calling.byPassPkgs;
    priorityConfig_.specialForeground.calling.filterUsages = config.specialForeground.calling.filterUsages;
    priorityConfig_.specialForeground.calling.filterPkgs = config.specialForeground.calling.filterPkgs;

    priorityConfig_.specialForeground.camera.byPassUsages = config.specialForeground.camera.byPassUsages;
    priorityConfig_.specialForeground.camera.byPassPkgs = config.specialForeground.camera.byPassPkgs;
    priorityConfig_.specialForeground.camera.filterUsages = config.specialForeground.camera.filterUsages;
    priorityConfig_.specialForeground.camera.filterPkgs = config.specialForeground.camera.filterPkgs;
}

VibrateStatus VibrationPriorityManager::ShouldIgnoreVibrate(const VibrateInfo &vibrateInfo,
    std::shared_ptr<VibratorThread> vibratorThread)
{
    if (IsPrivilegeApp(vibrateInfo.packageName)) {
        MISC_HILOGD("Privilege application, can vibrate");
        return VIBRATION;
    }
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

bool VibrationPriorityManager::IsPrivilegeApp(const std::string &name) const
{
    std::vector<std::string> privilegePkgs = priorityConfig_.privilegePkgs;
    auto it = std::find(privilegePkgs.begin(), privilegePkgs.end(), name);
    return (it != privilegePkgs.end());
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

int32_t VibrationPriorityManager::LoadPriorityConfig(const std::string &configPath, PriorityConfig &config)
{
    JsonParser parser(configPath);
    int32_t ret = parser.ParseJsonArray("privilegePkgs", config.privilegePkgs);
    CHKCR((ret == SUCCESS), ERROR, "Parse privilegePkgs fail");

    cJSON* trustLists = parser.GetObjectItem("trustLists");
    ret = ParserPriorityItem(parser, trustLists, "globalSettings", config.globalSettings);
    CHKCR((ret == SUCCESS), ERROR, "parse globalSettings fail");

    ret = ParserPriorityItem(parser, trustLists, "lowPowerMode", config.lowPowerMode);
    CHKCR((ret == SUCCESS), ERROR, "parse lowPowerMode fail");

    cJSON* specialForeground = parser.GetObjectItem(trustLists, "specialForeground");
    ret = ParserPriorityItem(parser, specialForeground, "calling", config.specialForeground.calling);
    CHKCR((ret == SUCCESS), ERROR, "parse calling fail");
    return ParserPriorityItem(parser, specialForeground, "camera", config.specialForeground.camera);
}

int32_t VibrationPriorityManager::ParserPriorityItem(const JsonParser &parser, cJSON *json, const std::string &key,
    PriorityItem &item)
{
    cJSON* cJson = parser.GetObjectItem(json, key);
    int32_t ret = parser.ParseJsonArray(cJson, "byPassUsages", item.byPassUsages);
    CHKCR((ret == SUCCESS), ERROR, "parse byPassUsages fail");
    ret = parser.ParseJsonArray(cJson, "byPassPkgs", item.byPassPkgs);
    CHKCR((ret == SUCCESS), ERROR, "parse byPassPkgs fail");
    ret = parser.ParseJsonArray(cJson, "filterUsages", item.filterUsages);
    CHKCR((ret == SUCCESS), ERROR, "parse filterUsages fail");
    return parser.ParseJsonArray(cJson, "filterPkgs", item.filterPkgs);
}
}  // namespace Sensors
}  // namespace OHOS