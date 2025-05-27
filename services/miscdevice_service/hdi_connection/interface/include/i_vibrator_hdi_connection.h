/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
#include "v2_0/ivibrator_interface.h"
#endif // HDF_DRIVERS_INTERFACE_VIBRATOR
#include "vibrator_infos.h"

namespace OHOS {
namespace Sensors {
#ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
using OHOS::HDI::Vibrator::V2_0::HdfVibratorMode;
using OHOS::HDI::Vibrator::V2_0::HDF_VIBRATOR_MODE_ONCE;
using OHOS::HDI::Vibrator::V2_0::HDF_VIBRATOR_MODE_PRESET;
using OHOS::HDI::Vibrator::V2_0::HDF_VIBRATOR_MODE_HDHAPTIC;
using OHOS::HDI::Vibrator::V2_0::HDF_VIBRATOR_MODE_BUTT;
using OHOS::HDI::Vibrator::V2_0::CurvePoint;
using OHOS::HDI::Vibrator::V2_0::DeviceVibratorInfo;
using OHOS::HDI::Vibrator::V2_0::EVENT_TYPE;
using OHOS::HDI::Vibrator::V2_0::HapticCapacity;
using OHOS::HDI::Vibrator::V2_0::HapticPaket;
using OHOS::HDI::Vibrator::V2_0::HapticEvent;
using OHOS::HDI::Vibrator::V2_0::HdfEffectInfo;
using OHOS::HDI::Vibrator::V2_0::HdfWaveInformation;
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
using OHOS::HDI::Vibrator::V2_0::HdfEffectType;
using OHOS::HDI::Vibrator::V2_0::HDF_EFFECT_TYPE_TIME;
using OHOS::HDI::Vibrator::V2_0::HDF_EFFECT_TYPE_PRIMITIVE;
using OHOS::HDI::Vibrator::V2_0::HDF_EFFECT_TYPE_BUTT;
using OHOS::HDI::Vibrator::V2_0::TimeEffect;
using OHOS::HDI::Vibrator::V2_0::PrimitiveEffect;
using OHOS::HDI::Vibrator::V2_0::CompositeEffect;
using OHOS::HDI::Vibrator::V2_0::HdfCompositeEffect;
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
#endif // HDF_DRIVERS_INTERFACE_VIBRATOR
class IVibratorHdiConnection {
public:
    IVibratorHdiConnection() = default;
    virtual ~IVibratorHdiConnection() = default;
    virtual int32_t ConnectHdi() = 0;
    virtual int32_t StartOnce(uint32_t duration) = 0;
    virtual int32_t Start(const std::string &effectType) = 0;
#ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    virtual int32_t EnableCompositeEffect(const HdfCompositeEffect &hdfCompositeEffect) = 0;
    virtual bool IsVibratorRunning() = 0;
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    virtual std::optional<HdfEffectInfo> GetEffectInfo(const std::string &effect) = 0;
    virtual int32_t Stop(HdfVibratorMode mode) = 0;
#endif // HDF_DRIVERS_INTERFACE_VIBRATOR
    virtual int32_t DestroyHdiConnection() = 0;
    virtual int32_t GetDelayTime(int32_t mode, int32_t &delayTime) = 0;
    virtual int32_t GetVibratorCapacity(VibratorCapacity &capacity) = 0;
    virtual int32_t PlayPattern(const VibratePattern &pattern) = 0;
    virtual int32_t StartByIntensity(const std::string &effect, int32_t intensity) = 0;
#ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
    virtual int32_t GetAllWaveInfo(std::vector<HdfWaveInformation> &waveInfos) = 0;
#endif // HDF_DRIVERS_INTERFACE_VIBRATOR

private:
    DISALLOW_COPY_AND_MOVE(IVibratorHdiConnection);
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // I_VIBRATOR_HDI_CONNECTION_H