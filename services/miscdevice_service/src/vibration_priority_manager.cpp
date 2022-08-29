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
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "VibrationPriorityManager" };
const std::string CONFIG_PATH = "/vendor/etc/sensors/vibration_priority_config.json";
}  // namespace

VibrationPriorityManager::VibrationPriorityManager()
{
    int32_t ret = LoadPriorityConfig(CONFIG_PATH);
    if (ret != SUCCESS) {
        MISC_HILOGE("lod priority config fail");
    }
}

VibrateStatus VibrationPriorityManager::ShouldIgnoreVibrate(VibrateInfo vibrateInfo,
    std::shared_ptr<VibratorThread> vibratorThread)
{
    std::vector<std::string> privilegePkgs = PriorityConfig_.privilegePkgs;
    if (std::find(privilegePkgs.begin(), privilegePkgs.end(), vibrateInfo.packageName) != privilegePkgs.end()) {
        MISC_HILOGD("privilege application, can vibrate");
        return VIBRATION;
    }
    if ((vibratorThread == nullptr) || (!vibratorThread->IsRunning())) {
        MISC_HILOGD("can vibrate");
        return VIBRATION;
    }
    if (vibrateInfo.mode == "preset" && (vibrateInfo.count > 1)) {
        MISC_HILOGD("can vibrate, loop priority is high");
        return VIBRATION;
    }
    const VibrateInfo info = vibratorThread->GetCurrentVibrateInfo();
    if (info.usage == USAGE_ALARM) {
        MISC_HILOGD("vibration is ignored for alarm");
        return IGNORE_FOR_ALARM;
    }
    if (info.mode == "preset" && (info.count > 1)) {
        MISC_HILOGD("vibration is ignored for repeat");
        return IGNORE_FOR_REPEAT;
    }
    if ((info.usage != vibrateInfo.usage) && (vibrateInfo.usage == USAGE_UNKNOWN)) {
        MISC_HILOGD("vibration is ignored, unknown has a low priority");
        return IGNORE_FOR_UNKNOWN;
    }
    return VIBRATION;
}

int32_t VibrationPriorityManager::LoadPriorityConfig(const std::string &configPath)
{
    jsonParser_ = std::make_unique<MiscdeviceJsonParser>(configPath);
    cJSON* cJson = jsonParser_->GetJsonData();
    int32_t ret = jsonParser_->ParseJsonArray(cJson, "privilegePkgs", PriorityConfig_.privilegePkgs);
    CHKCR((ret == SUCCESS), ERROR, "parse privilegePkgs fail");

    cJSON* trustLists = jsonParser_->GetObjectItem(cJson, "trustLists");
    ret = ParserPriorityItem(trustLists, "globalSettings", PriorityConfig_.globalSettings);
    CHKCR((ret == SUCCESS), ERROR, "parse globalSettings fail");

    ret = ParserPriorityItem(trustLists, "lowPowerMode", PriorityConfig_.lowPowerMode);
    CHKCR((ret == SUCCESS), ERROR, "parse lowPowerMode fail");

    cJSON* specialForeground = jsonParser_->GetObjectItem(trustLists, "specialForeground");
    ret = ParserPriorityItem(specialForeground, "calling", PriorityConfig_.specialForeground.calling);
    CHKCR((ret == SUCCESS), ERROR, "parse calling fail");
    ret = ParserPriorityItem(specialForeground, "camera", PriorityConfig_.specialForeground.camera);
    CHKCR((ret == SUCCESS), ERROR, "parse camera fail");
    return SUCCESS;
}

int32_t VibrationPriorityManager::ParserPriorityItem(cJSON *json, const std::string& key,
    PriorityItem& item)
{
    cJSON* cJson = jsonParser_->GetObjectItem(json, key);
    int32_t ret = jsonParser_->ParseJsonArray(cJson, "byPassUsages", item.byPassUsages);
    CHKCR((ret == SUCCESS), ERROR, "parse byPassUsages fail");
    ret = jsonParser_->ParseJsonArray(cJson, "byPassPkgs", item.byPassPkgs);
    CHKCR((ret == SUCCESS), ERROR, "parse byPassPkgs fail");
    ret = jsonParser_->ParseJsonArray(cJson, "filterUsages", item.filterUsages);
    CHKCR((ret == SUCCESS), ERROR, "parse filterUsages fail");
    ret = jsonParser_->ParseJsonArray(cJson, "filterPkgs", item.filterPkgs);
    CHKCR((ret == SUCCESS), ERROR, "parse filterPkgs fail");
    return SUCCESS;
}
}  // namespace Sensors
}  // namespace OHOS