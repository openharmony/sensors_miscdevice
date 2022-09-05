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

#ifndef I_MISCDEVICE_SERVICE_H
#define I_MISCDEVICE_SERVICE_H

#include <string>
#include <vector>

#include "iremote_broker.h"
#include "miscdevice_common.h"

namespace OHOS {
namespace Sensors {
class IMiscdeviceService : public IRemoteBroker {
public:
    IMiscdeviceService() = default;

    virtual ~IMiscdeviceService() = default;

    DECLARE_INTERFACE_DESCRIPTOR(u"IMiscdeviceService");

    virtual bool IsAbilityAvailable(MiscdeviceDeviceId groupID) = 0;

    virtual bool IsVibratorEffectAvailable(int32_t vibratorId, const std::string &effectType) = 0;

    virtual std::vector<int32_t> GetVibratorIdList() = 0;

    virtual int32_t Vibrate(int32_t vibratorId, int32_t timeOut, int32_t usage) = 0;

    virtual int32_t CancelVibrator(int32_t vibratorId) = 0;

    virtual int32_t PlayVibratorEffect(int32_t vibratorId, const std::string &effect,
                                       int32_t loopCount, int32_t usage) = 0;

    virtual int32_t PlayCustomVibratorEffect(int32_t vibratorId, const std::vector<int32_t> &timing,
                                             const std::vector<int32_t> &intensity, int32_t periodCount) = 0;

    virtual int32_t StopVibratorEffect(int32_t vibratorId, const std::string &effect) = 0;

    virtual int32_t SetVibratorParameter(int32_t vibratorId, const std::string &cmd) = 0;

    virtual std::string GetVibratorParameter(int32_t vibratorId, const std::string &cmd) = 0;

    virtual std::vector<int32_t> GetLightSupportId() = 0;

    virtual bool IsLightEffectSupport(int32_t lightId, const std::string &effectId) = 0;

    virtual int32_t Light(int32_t lightId, uint64_t brightness, uint32_t timeOn, uint32_t timeOff) = 0;

    virtual int32_t PlayLightEffect(int32_t lightId, const std::string &type) = 0;

    virtual int32_t StopLightEffect(int32_t lightId) = 0;

    enum {
        IS_ABILITY_AVAILABLE = 0,
        GET_VIBRATOR_ID_LIST,
        IS_VIBRATOR_EFFECT_AVAILABLE,
        VIBRATE,
        CANCEL_VIBRATOR,
        PLAY_VIBRATOR_EFFECT,
        PLAY_CUSTOM_VIBRATOR_EFFECT,
        STOP_VIBRATOR_EFFECT,
        SET_VIBRATOR_PARA,
        GET_VIBRATOR_PARA,
        GET_LIGHT_SUPPORT_ID,
        IS_LIGHT_EFFECT_SUPPORT,
        LIGHT,
        PLAY_LIGHT_EFFECT,
        STOP_LIGHT_EFFECT,
    };
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // I_MISCDEVICE_SERVICE_H
