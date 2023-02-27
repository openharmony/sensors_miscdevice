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

#ifndef I_VIBRATOR_HDI_CONNECTION_H
#define I_VIBRATOR_HDI_CONNECTION_H

#include <optional>
#include <stdint.h>
#include <string>

#include <nocopyable.h>
#include "v1_1/vibrator_types.h"

namespace OHOS {
namespace Sensors {
using OHOS::HDI::Vibrator::V1_1::HdfVibratorMode;
using OHOS::HDI::Vibrator::V1_1::HDF_VIBRATOR_MODE_ONCE;
using OHOS::HDI::Vibrator::V1_1::HDF_VIBRATOR_MODE_PRESET;
using OHOS::HDI::Vibrator::V1_1::HDF_VIBRATOR_MODE_BUTT;
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
using OHOS::HDI::Vibrator::V1_1::HdfEffectType;
using OHOS::HDI::Vibrator::V1_1::HDF_EFFECT_TYPE_TIME;
using OHOS::HDI::Vibrator::V1_1::HDF_EFFECT_TYPE_PRIMITIVE;
using OHOS::HDI::Vibrator::V1_1::HDF_EFFECT_TYPE_BUTT;
using OHOS::HDI::Vibrator::V1_1::TimeEffect;
using OHOS::HDI::Vibrator::V1_1::PrimitiveEffect;
using OHOS::HDI::Vibrator::V1_1::CompositeEffect;
using OHOS::HDI::Vibrator::V1_1::HdfCompositeEffect;
using OHOS::HDI::Vibrator::V1_1::HdfEffectInfo;
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
class IVibratorHdiConnection {
public:
    IVibratorHdiConnection() = default;
    virtual ~IVibratorHdiConnection() = default;
    virtual int32_t ConnectHdi() = 0;
    virtual int32_t StartOnce(uint32_t duration) = 0;
    virtual int32_t Start(const std::string &effectType) = 0;
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    virtual int32_t EnableCompositeEffect(const HdfCompositeEffect &hdfCompositeEffect) = 0;
    virtual bool IsVibratorRunning() = 0;
    virtual std::optional<HdfEffectInfo> GetEffectInfo(const std::string &effect) = 0;
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    virtual int32_t Stop(HdfVibratorMode mode) = 0;
    virtual int32_t DestroyHdiConnection() = 0;

private:
    DISALLOW_COPY_AND_MOVE(IVibratorHdiConnection);
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // I_VIBRATOR_HDI_CONNECTION_H