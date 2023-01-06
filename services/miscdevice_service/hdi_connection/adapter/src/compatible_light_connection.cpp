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
#include "compatible_light_connection.h"

#include <ctime>
#include <string>
#include <vector>

#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "CompatibleLightConnection" };
}
std::vector<LightInfo> lightInfo = {
    {"light_test", 1, 3, 1}
};
std::vector<int32_t> supportLights = { 1 };

int32_t CompatibleLightConnection::ConnectHdi()
{
    CALL_LOG_ENTER;
    return ERR_OK;
}

int32_t CompatibleLightConnection::GetLightList(std::vector<LightInfo>& lightList) const
{
    CALL_LOG_ENTER;
    lightList.assign(lightInfo.begin(), lightInfo.end());
    return ERR_OK;
}

int32_t CompatibleLightConnection::TurnOn(int32_t lightId,  const LightColor &color, const LightAnimation &animation)
{
    CALL_LOG_ENTER;
    if (std::find(supportLights.begin(), supportLights.end(), lightId) == supportLights.end()) {
        MISC_HILOGE("not support TurnOn lightId:%{public}d", lightId);
        return LIGHT_ID_NOT_SUPPORT;
    }
    if ((animation.mode == LIGHT_MODE_BLINK || animation.mode == LIGHT_MODE_GRADIENT) &&
        (animation.onTime <= 0 || animation.offTime <= 0)) {
        MISC_HILOGE("animation parameter error");
        return LIGHT_ERR;
    }
    if (std::find(turnOnLights_.begin(), turnOnLights_.end(), lightId) != turnOnLights_.end()) {
        MISC_HILOGI("lightId:%{public}d has been turnOn", lightId);
        return ERR_OK;
    }
    turnOnLights_.push_back(lightId);
    return ERR_OK;
}

int32_t CompatibleLightConnection::TurnOff(int32_t lightId)
{
    CALL_LOG_ENTER;
    if (std::find(supportLights.begin(), supportLights.end(), lightId) == supportLights.end()) {
        MISC_HILOGE("not support TurnOff lightId:%{public}d", lightId);
        return LIGHT_ID_NOT_SUPPORT;
    }
    if (std::find(turnOnLights_.begin(), turnOnLights_.end(), lightId) == turnOnLights_.end()) {
        MISC_HILOGE("lightId:%{public}d should not be turn off", lightId);
        return LIGHT_END_ERROR;
    }
    std::vector<int32_t>::iterator iter = std::find(turnOnLights_.begin(), turnOnLights_.end(), lightId);
    turnOnLights_.erase(iter);
    return ERR_OK;
}

int32_t CompatibleLightConnection::DestroyHdiConnection()
{
    CALL_LOG_ENTER;
    return ERR_OK;
}
}  // namespace Sensors
}  // namespace OHOS
