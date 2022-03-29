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

#ifndef I_VIBRATOR_HDI_CONNECTION_H
#define I_VIBRATOR_HDI_CONNECTION_H
#include <nocopyable.h>
#include <stdint.h>
namespace OHOS {
namespace Sensors {
class IVibratorHdiConnection {
public:
    enum VibratorStopMode {
        VIBRATOR_STOP_MODE_TIME   = 0,    /**< Indicates the one-shot vibration with the given duration. */
        VIBRATOR_STOP_MODE_PRESET = 1,    /**< Indicates the periodic vibration with the preset effect. */
        VIBRATOR_STOP_MODE_INVALID        /**< Indicates invalid the effect mode. */
    };

    IVibratorHdiConnection() = default;

    virtual ~IVibratorHdiConnection() = default;

    virtual int32_t ConnectHdi() = 0;

    virtual int32_t StartOnce(uint32_t duration) = 0;

    virtual int32_t Start(const char *effectType) = 0;

    virtual int32_t Stop(VibratorStopMode mode) = 0;

    virtual int32_t DestroyHdiConnection() = 0;

private:
    DISALLOW_COPY_AND_MOVE(IVibratorHdiConnection);
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // I_VIBRATOR_HDI_CONNECTION_H