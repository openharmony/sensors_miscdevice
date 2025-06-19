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

#ifndef VIBRATOR_THREAD_H
#define VIBRATOR_THREAD_H

#include <thread>

#include "thread_ex.h"

#include "vibrator_hdi_connection.h"

namespace OHOS {
namespace Sensors {
class VibratorThread : public Thread {
public:
    void UpdateVibratorEffect(const VibrateInfo &vibrateInfo, const VibratorIdentifierIPC& identifier,
        std::vector<HdfWaveInformation> &waveInfos);
    VibrateInfo GetCurrentVibrateInfo();
    std::vector<HdfWaveInformation> GetCurrentWaveInfo();
    void SetExitStatus(bool status);
    void WakeUp();
    void ResetVibrateInfo();
protected:
    virtual bool Run();

private:
    VibratorIdentifierIPC GetCurrentVibrateParams();
    int32_t PlayOnce(const VibrateInfo &info, const VibratorIdentifierIPC& identifier);
    int32_t PlayEffect(const VibrateInfo &info, const VibratorIdentifierIPC& identifier);
    int32_t PlayCustomByHdHptic(const VibrateInfo &info, const VibratorIdentifierIPC& identifier);
    void HandleMultipleVibrations(const VibratorIdentifierIPC& identifier);
    VibrateInfo copyInfoWithIndexEvents(const VibrateInfo& originalInfo, const VibratorIdentifierIPC& identifier);
#ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
    int32_t PlayCustomByCompositeEffect(const VibrateInfo &info, const VibratorIdentifierIPC& identifier,
        const std::vector<HdfWaveInformation> &waveInfo);
    int32_t PlayCompositeEffect(const VibrateInfo &info, const HdfCompositeEffect &hdfCompositeEffect,
        const VibratorIdentifierIPC& identifier);
#endif // HDF_DRIVERS_INTERFACE_VIBRATOR
    std::mutex currentVibrationMutex_;
    VibrateInfo currentVibration_;
    std::vector<HdfWaveInformation> waveInfos_;
    std::mutex currentVibrateParamsMutex_;
    VibratorIdentifierIPC currentVibrateParams_;
    std::mutex vibrateMutex_;
    std::condition_variable cv_;
    std::atomic<bool> exitFlag_ = false;
};
#define VibratorDevice VibratorHdiConnection::GetInstance()
}  // namespace Sensors
}  // namespace OHOS
#endif  // VIBRATOR_THREAD_H