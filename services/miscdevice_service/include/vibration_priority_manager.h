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

#ifndef VIBRATION_PRIORITY_MANAGER_H
#define VIBRATION_PRIORITY_MANAGER_H

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "datashare_helper.h"
#include "iremote_object.h"
#include "singleton.h"

#include "json_parser.h"
#include "miscdevice_ringer_mode_callback.h"
#include "miscdevice_observer.h"
#include "vibrator_infos.h"
#include "vibrator_thread.h"

namespace OHOS {
namespace Sensors {
enum VibrateStatus {
    VIBRATION = 0,
    IGNORE_BACKGROUND = 1,
    IGNORE_LOW_POWER = 2,
    IGNORE_GLOBAL_SETTINGS = 3,
    IGNORE_RINGTONE = 4,
    IGNORE_REPEAT = 5,
    IGNORE_ALARM = 6,
    IGNORE_UNKNOWN = 7,
    IGNORE_RINGER_MODE = 8,
    IGNORE_FEEDBACK = 9,
};

class VibrationPriorityManager {
    DECLARE_DELAYED_SINGLETON(VibrationPriorityManager);
public:
    DISALLOW_COPY_AND_MOVE(VibrationPriorityManager);
    VibrateStatus ShouldIgnoreVibrate(const VibrateInfo &vibrateInfo, std::shared_ptr<VibratorThread> vibratorThread);
    static void ExecRegisterCb(const sptr<MiscDeviceObserver> &observer);
    int32_t RegisterObserver(const sptr<MiscDeviceObserver> &observer);
    int32_t UnregisterObserver(const sptr<MiscDeviceObserver> &observer);

private:
    bool IsCurrentVibrate(std::shared_ptr<VibratorThread> vibratorThread) const;
    bool IsLoopVibrate(const VibrateInfo &vibrateInfo) const;
    VibrateStatus ShouldIgnoreVibrate(const VibrateInfo &vibrateInfo, VibrateInfo currentVibrateInfo) const;
    int32_t GetIntValue(const std::string &key, int32_t &value);
    int32_t GetLongValue(const std::string &key, int64_t &value);
    int32_t GetStringValue(const std::string &key, std::string &value);
    Uri AssembleUri(const std::string &key);
    std::shared_ptr<DataShare::DataShareHelper> CreateDataShareHelper();
    bool ReleaseDataShareHelper(std::shared_ptr<DataShare::DataShareHelper> &helper);
    sptr<MiscDeviceObserver> CreateObserver(const std::string &key, MiscDeviceObserver::UpdateFunc &func);
    static void Initialize();
    void UpdateStatus();
    static sptr<IRemoteObject> remoteObj_;
    sptr<MiscDeviceObserver> observer_ { nullptr };
    std::shared_ptr<MiscDeviceRingerModeCallback> ringerModeCB_ { nullptr };
    std::atomic_int32_t miscFeedback_ { -1 };
    std::atomic_int32_t miscAudioRingerMode_ { -1 };
};
#define PriorityManager DelayedSingleton<VibrationPriorityManager>::GetInstance()
}  // namespace Sensors
}  // namespace OHOS
#endif  // VIBRATION_PRIORITY_MANAGER_H