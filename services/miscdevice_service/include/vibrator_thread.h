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

#include <condition_variable>
#include <cstdint>
#include <thread>

#include "thread_ex.h"
#include "vibrator_infos.h"

namespace OHOS {
namespace Sensors {
class VibratorThread : public Thread {
public:
    void UpdateVibratorEffect(VibrateInfo vibrateInfo);
    VibrateInfo GetCurrentVibrateInfo();
    void NotifyExit();

protected:
    virtual bool Run();

private:
    void SetReadyStatus(bool status);
    VibrateInfo currentVibration_;
    std::condition_variable cv_;
    std::mutex currentVibrationMutex_;
    std::mutex readyMutex_;
    std::mutex vibrateMutex_;
    bool ready_ = false;
};
#define VibratorDevice VibratorHdiConnection::GetInstance()
}  // namespace Sensors
}  // namespace OHOS
#endif  // VIBRATOR_THREAD_H