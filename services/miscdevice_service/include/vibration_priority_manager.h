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

#include "app_mgr_client.h"
#ifdef OHOS_BUILD_ENABLE_DO_NOT_DISTURB
#include "cJSON.h"
#endif // OHOS_BUILD_ENABLE_DO_NOT_DISTURB
#include "datashare_helper.h"

#include "miscdevice_observer.h"
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
    IGNORE_RINGER_VIBRATE_WHEN_RING = 10,
};

enum RingerMode {
    RINGER_MODE_INVALID = -1,
    RINGER_MODE_SILENT = 0,
    RINGER_MODE_VIBRATE = 1,
    RINGER_MODE_NORMAL = 2
};

enum FeedbackMode {
    FEEDBACK_MODE_INVALID = -1,
    FEEDBACK_MODE_OFF = 0,
    FEEDBACK_MODE_ON = 1
};

enum VibrateWhenRingMode {
    VIBRATE_WHEN_RING_MODE_INVALID = -1,
    VIBRATE_WHEN_RING_MODE_OFF = 0,
    VIBRATE_WHEN_RING_MODE_ON = 1
};

#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CROWN
enum FeedbackIntensity {
    FEEDBACK_INTENSITY_INVALID = -1,
    FEEDBACK_INTENSITY_STRONGE = 0,
    FEEDBACK_INTENSITY_WEAK = 1,
    FEEDBACK_INTENSITY_NONE = 2,
};
#endif

#ifdef OHOS_BUILD_ENABLE_DO_NOT_DISTURB
enum DoNotDisturbSwitch {
    DONOTDISTURB_SWITCH_INVALID = -1,
    DONOTDISTURB_SWITCH_OFF = 0,
    DONOTDISTURB_SWITCH_ON = 1
};

struct WhiteListAppInfo {
    std::string bundle;
    int64_t uid;
};
#endif // OHOS_BUILD_ENABLE_DO_NOT_DISTURB

class VibrationPriorityManager {
    DECLARE_DELAYED_SINGLETON(VibrationPriorityManager);
public:
    DISALLOW_COPY_AND_MOVE(VibrationPriorityManager);
    bool Init();
    static bool IsSystemServiceCalling();
    static bool IsSystemCalling();
    VibrateStatus ShouldIgnoreVibrate(const VibrateInfo &vibrateInfo,
		const std::shared_ptr<VibratorThread> &vibratorThread, const VibratorIdentifierIPC& identifier);
#ifdef OHOS_BUILD_ENABLE_DO_NOT_DISTURB
    void InitDoNotDisturbData();
    void ReregisterCurrentUserObserver();
#endif // OHOS_BUILD_ENABLE_DO_NOT_DISTURB
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CROWN
    bool ShouldIgnoreByIntensity(const VibrateInfo &vibrateInfo);
    void MiscCrownIntensityFeedbackInit(void);
#endif

private:
    bool IsCurrentVibrate(const std::shared_ptr<VibratorThread> &vibratorThread,
        const VibratorIdentifierIPC& identifier) const;
    bool IsLoopVibrate(const VibrateInfo &vibrateInfo) const;
    VibrateStatus ShouldIgnoreVibrate(const VibrateInfo &vibrateInfo, VibrateInfo currentVibrateInfo) const;
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_INPUT_METHOD
    bool ShouldIgnoreInputMethod(const VibrateInfo &vibrateInfo);
#endif // OHOS_BUILD_ENABLE_VIBRATOR_INPUT_METHOD
    static void ExecRegisterCb(const sptr<MiscDeviceObserver> &observer);
    int32_t RegisterObserver(const sptr<MiscDeviceObserver> &observer);
    int32_t UnregisterObserver(const sptr<MiscDeviceObserver> &observer);
    int32_t GetIntValue(const std::string &uri, const std::string &key, int32_t &value);
    int32_t GetLongValue(const std::string &uri, const std::string &key, int64_t &value);
    int32_t GetStringValue(const std::string &uri, const std::string &key, std::string &value);
#ifdef OHOS_BUILD_ENABLE_DO_NOT_DISTURB
    int32_t GetDoNotDisturbStringValue(const std::string &key, std::string &value);
    int32_t GetDoNotDisturbIntValue(const std::string &key, int32_t &value);
    int32_t GetDoNotDisturbLongValue(const std::string &key, int64_t &value);
    int32_t GetWhiteListValue(const std::string &key, std::vector<WhiteListAppInfo> &value);
    void DeleteCJSONValue(cJSON *jsonValue);
    bool IgnoreAppVibrations(const VibrateInfo &vibrateInfo);
    void UpdateCurrentUserId();
    int32_t RegisterUserObserver();
    int32_t UnregisterUserObserver();
    std::string ReplaceUserIdForUri(std::string uri, int32_t userId);
    Uri DoNotDisturbAssembleUri(const std::string &key);
#endif // OHOS_BUILD_ENABLE_DO_NOT_DISTURB
    Uri AssembleUri(const std::string &uri, const std::string &key);
    std::shared_ptr<DataShare::DataShareHelper> CreateDataShareHelper(const std::string &tableUrl);
    bool ReleaseDataShareHelper(std::shared_ptr<DataShare::DataShareHelper> &helper);
    sptr<MiscDeviceObserver> CreateObserver(const MiscDeviceObserver::UpdateFunc &func);
    void UpdateStatus();
    void ReportSwitchStatus();
    std::condition_variable stopCondition_;
    std::thread reportSwitchStatusThread_;
    static std::atomic_bool stop_;
    static std::mutex stopMutex_;
    sptr<IRemoteObject> remoteObj_ { nullptr };
    sptr<MiscDeviceObserver> observer_ { nullptr };
    std::shared_ptr<AppExecFwk::AppMgrClient> appMgrClientPtr_ {nullptr};
    std::atomic_int32_t miscFeedback_ = FEEDBACK_MODE_INVALID;
    std::atomic_int32_t miscAudioRingerMode_ = RINGER_MODE_INVALID;
    std::atomic_int32_t vibrateWhenRing_ = VIBRATE_WHEN_RING_MODE_INVALID;
#ifdef OHOS_BUILD_ENABLE_DO_NOT_DISTURB
    std::atomic_int32_t doNotDisturbSwitch_ = DONOTDISTURB_SWITCH_INVALID;
    std::vector<WhiteListAppInfo> doNotDisturbWhiteList_;
    sptr<MiscDeviceObserver> currentUserObserver_;
    std::mutex currentUserObserverMutex_;
    std::mutex whiteListMutex_;
#endif // OHOS_BUILD_ENABLE_DO_NOT_DISTURB
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CROWN
    std::atomic_int32_t miscCrownFeedback_ = FEEDBACK_MODE_INVALID;
    std::atomic_int32_t miscIntensity_ = FEEDBACK_INTENSITY_INVALID;
#endif
};
#define PriorityManager DelayedSingleton<VibrationPriorityManager>::GetInstance()
}  // namespace Sensors
}  // namespace OHOS
#endif  // VIBRATION_PRIORITY_MANAGER_H