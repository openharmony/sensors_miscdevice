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

#ifndef DIRECT_CONNECTION_H
#define DIRECT_CONNECTION_H

#include <thread>

#include "i_vibrator_hdi_connection.h"

namespace OHOS {
namespace Sensors {
class CompatibleConnection : public IVibratorHdiConnection {
public:
    CompatibleConnection() = default;
    virtual ~CompatibleConnection() {};
    int32_t ConnectHdi() override;
    int32_t StartOnce(const VibratorIdentifierIPC &identifier, uint32_t duration) override;
    int32_t Start(const VibratorIdentifierIPC &identifier, const std::string &effectType) override;
#ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    int32_t EnableCompositeEffect(const VibratorIdentifierIPC &identifier,
        const HdfCompositeEffect &hdfCompositeEffect) override;
    bool IsVibratorRunning(const VibratorIdentifierIPC &identifier) override;
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    std::optional<HdfEffectInfo> GetEffectInfo(const VibratorIdentifierIPC &identifier,
        const std::string &effect) override;
    int32_t Stop(const VibratorIdentifierIPC &identifier, HdfVibratorMode mode) override;
#endif // HDF_DRIVERS_INTERFACE_VIBRATOR
    int32_t GetDelayTime(const VibratorIdentifierIPC &identifier, int32_t mode, int32_t &delayTime) override;
    int32_t GetVibratorCapacity(const VibratorIdentifierIPC &identifier, VibratorCapacity &capacity) override;
    int32_t PlayPattern(const VibratorIdentifierIPC &identifier, const VibratePattern &pattern) override;
    int32_t PlayPatternBySessionId(const VibratorIdentifierIPC &identifier, uint32_t sessionId,
        const VibratePattern &pattern) override;
    int32_t PlayPackageBySessionId(const VibratorIdentifierIPC &identifier, uint32_t sessionId,
        const VibratePackageIPC &package) override;
    int32_t StopVibrateBySessionId(const VibratorIdentifierIPC &identifier, uint32_t sessionId) override;
    int32_t DestroyHdiConnection() override;
    int32_t StartByIntensity(const VibratorIdentifierIPC &identifier, const std::string &effect,
        int32_t intensity) override;
#ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
    int32_t GetVibratorInfo(std::vector<HdfVibratorInfo> &hdfVibratorInfo) override;
    int32_t GetAllWaveInfo(const VibratorIdentifierIPC &identifier,
        std::vector<HdfWaveInformation> &waveInfos) override;
    int32_t GetVibratorList(const VibratorIdentifierIPC &identifier,
        std::vector<HdfVibratorInfo> &vibratorInfoIpc) override;
    int32_t GetEffectInfo(const VibratorIdentifierIPC &identifier, const std::string &effectType,
        HdfEffectInfo &effectInfo) override;
#endif // HDF_DRIVERS_INTERFACE_VIBRATOR
    int32_t RegisterVibratorPlugCallback(DevicePlugCallback cb) override;
    DevicePlugCallback GetVibratorPlugCb() override;

private:
    DISALLOW_COPY_AND_MOVE(CompatibleConnection);
    std::thread vibrateThread_;
    static void VibrateProcess();
    static int32_t duration_;
    static std::atomic_bool isStop_;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // DIRECT_CONNECTION_H