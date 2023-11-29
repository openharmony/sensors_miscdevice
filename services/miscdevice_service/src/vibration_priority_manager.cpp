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

#include "vibration_priority_manager.h"

#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "uri.h"

#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
sptr<IRemoteObject> VibrationPriorityManager::remoteObj_ { nullptr };
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "VibrationPriorityManager" };
const std::string SETTING_COLUMN_KEYWORD = "KEYWORD";
const std::string SETTING_COLUMN_VALUE = "VALUE";
const std::string SETTING_SOUND_FEEDBACK_KEY = "physic_navi_haptic_feedback_enabled";
const std::string SETTING_URI_PROXY = "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
constexpr const char *SETTINGS_DATA_EXT_URI = "datashare:///com.ohos.settingsdata.DataAbility";
constexpr int32_t DECEM_BASE = 10;
}  // namespace

VibrationPriorityManager::VibrationPriorityManager()
{
    Initialize();
    MiscDeviceObserver::UpdateFunc updateFunc = [&](const std::string &key) {
        int32_t feedback { -1 };
        if (GetIntValue(key, feedback) != ERR_OK) {
            MISC_HILOGE("Get feedback failed");
            return;
        }
        miscFeedback_ = feedback;
        MISC_HILOGI("feedback:%{public}d", feedback);
    };
    auto observer = CreateObserver(SETTING_SOUND_FEEDBACK_KEY, updateFunc);
    if (observer == nullptr) {
        MISC_HILOGE("observer is null");
        return;
    }
    observer_ = observer;
    if (RegisterObserver(observer_) != ERR_OK) {
        MISC_HILOGE("RegisterObserver failed");
    }

    ringerModeCB_ = std::make_shared<MiscDeviceRingerModeCallback>();
    AudioStandard::AudioSystemManager::GetInstance()->GetGroupManager(
        AudioStandard::DEFAULT_VOLUME_GROUP_ID)->SetRingerModeCallback(IPCSkeleton::GetCallingPid(), ringerModeCB_);
}

VibrationPriorityManager::~VibrationPriorityManager()
{
    remoteObj_ = nullptr;
    if (UnregisterObserver(observer_) != ERR_OK) {
        MISC_HILOGE("UnregisterObserver failed");
    }
    int32_t ret = AudioStandard::AudioSystemManager::GetInstance()->GetGroupManager(
        AudioStandard::DEFAULT_VOLUME_GROUP_ID)->UnsetRingerModeCallback(IPCSkeleton::GetCallingPid());
    if (ret != ERR_OK) {
        MISC_HILOGE("UnSetRingerModeCallback failed");
    }
}

int32_t VibrationPriorityManager::GetIntValue(const std::string &key, int32_t &value)
{
    int64_t valueLong;
    int32_t ret = GetLongValue(key, valueLong);
    if (ret != ERR_OK) {
        return ret;
    }
    value = static_cast<int32_t>(valueLong);
    return ERR_OK;
}

int32_t VibrationPriorityManager::GetLongValue(const std::string &key, int64_t &value)
{
    std::string valueStr;
    int32_t ret = GetStringValue(key, valueStr);
    if (ret != ERR_OK) {
        return ret;
    }
    value = static_cast<int64_t>(strtoll(valueStr.c_str(), nullptr, DECEM_BASE));
    return ERR_OK;
}

int32_t VibrationPriorityManager::GetStringValue(const std::string &key, std::string &value)
{
    std::string callingIdentity = IPCSkeleton::ResetCallingIdentity();
    auto helper = CreateDataShareHelper();
    if (helper == nullptr) {
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return MISC_NO_INIT_ERR;
    }
    std::vector<std::string> columns = {SETTING_COLUMN_VALUE};
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SETTING_COLUMN_KEYWORD, key);
    Uri uri(AssembleUri(key));
    auto resultSet = helper->Query(uri, predicates, columns);
    ReleaseDataShareHelper(helper);
    if (resultSet == nullptr) {
        MISC_HILOGE("resultSet is nullptr");
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return MISC_INVALID_OPERATION_ERR;
    }
    int32_t count;
    resultSet->GetRowCount(count);
    if (count == 0) {
        MISC_HILOGW("Not found value, key:%{public}s, count:%{public}d", key.c_str(), count);
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return MISC_NAME_NOT_FOUND_ERR;
    }
    const int32_t index = 0;
    resultSet->GoToRow(index);
    int32_t ret = resultSet->GetString(index, value);
    if (ret != ERR_OK) {
        MISC_HILOGW("GetString failed, ret:%{public}d", ret);
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERROR;
    }
    resultSet->Close();
    IPCSkeleton::SetCallingIdentity(callingIdentity);
    return ERR_OK;
}

VibrateStatus VibrationPriorityManager::ShouldIgnoreVibrate(const VibrateInfo &vibrateInfo,
    std::shared_ptr<VibratorThread> vibratorThread)
{
    if (ringerModeCB_ != nullptr) {
        int32_t ringerMode = static_cast<int32_t>(ringerModeCB_->GetAudioRingerMode());
        if (ringerMode != -1) {
            miscAudioRingerMode_ = ringerMode;
        } else {
            miscAudioRingerMode_ = AudioStandard::AudioSystemManager::GetInstance()->GetGroupManager(
                AudioStandard::DEFAULT_VOLUME_GROUP_ID)->GetRingerMode();
        }
    } else {
        MISC_HILOGE("ringerModeCB_ is nullptr");
    }
    if (miscFeedback_ == -1) {
        int32_t feedback = -1;
        if (GetIntValue(SETTING_SOUND_FEEDBACK_KEY, feedback) != ERR_OK) {
            MISC_HILOGD("Get feedback failed");
        }
        miscFeedback_ = feedback;
    }
    if ((vibrateInfo.usage == USAGE_ALARM || vibrateInfo.usage == USAGE_RING || vibrateInfo.usage == USAGE_NOTIFICATION
        || vibrateInfo.usage == USAGE_COMMUNICATION) && (miscAudioRingerMode_ == 0)) {
        MISC_HILOGD("Vibration is ignored for ringer mode:%{public}d", static_cast<int32_t>(miscAudioRingerMode_));
        return IGNORE_RINGER_MODE;
    }
    if ((vibrateInfo.usage == USAGE_TOUCH || vibrateInfo.usage == USAGE_MEDIA || vibrateInfo.usage == USAGE_UNKNOWN
        || vibrateInfo.usage == USAGE_PHYSICAL_FEEDBACK || vibrateInfo.usage == USAGE_SIMULATE_REALITY)
        && (miscFeedback_ == 0)) {
        MISC_HILOGD("Vibration is ignored for ringer mode:%{public}d", static_cast<int32_t>(miscAudioRingerMode_));
        return IGNORE_FEEDBACK;
    }
    if (vibratorThread == nullptr) {
        MISC_HILOGD("There is no vibration, it can vibrate");
        return VIBRATION;
    }
    if (!IsCurrentVibrate(vibratorThread)) {
        MISC_HILOGD("There is no vibration at the moment, it can vibrate");
        return VIBRATION;
    }
    if (IsLoopVibrate(vibrateInfo)) {
        MISC_HILOGD("Can vibrate, loop priority is high");
        return VIBRATION;
    }
    return ShouldIgnoreVibrate(vibrateInfo, vibratorThread->GetCurrentVibrateInfo());
}

bool VibrationPriorityManager::IsCurrentVibrate(std::shared_ptr<VibratorThread> vibratorThread) const
{
#if defined(OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM)
    return ((vibratorThread != nullptr) && (vibratorThread->IsRunning() || VibratorDevice.IsVibratorRunning()));
#else
    return ((vibratorThread != nullptr) && (vibratorThread->IsRunning()));
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
}

bool VibrationPriorityManager::IsLoopVibrate(const VibrateInfo &vibrateInfo) const
{
    return ((vibrateInfo.mode == "preset") && (vibrateInfo.count > 1));
}

VibrateStatus VibrationPriorityManager::ShouldIgnoreVibrate(const VibrateInfo &vibrateInfo,
    VibrateInfo currentVibrateInfo) const
{
    if (currentVibrateInfo.usage == USAGE_ALARM) {
        MISC_HILOGD("Vibration is ignored for alarm");
        return IGNORE_ALARM;
    }
    if (IsLoopVibrate(currentVibrateInfo)) {
        MISC_HILOGD("Vibration is ignored for repeat");
        return IGNORE_REPEAT;
    }
    if ((currentVibrateInfo.usage != vibrateInfo.usage) && (vibrateInfo.usage == USAGE_UNKNOWN)) {
        MISC_HILOGD("Vibration is ignored, unknown has a low priority");
        return IGNORE_UNKNOWN;
    }
    return VIBRATION;
}

sptr<MiscDeviceObserver> VibrationPriorityManager::CreateObserver(const std::string &key,
    MiscDeviceObserver::UpdateFunc &func)
{
    sptr<MiscDeviceObserver> observer = new MiscDeviceObserver();
    if (observer == nullptr) {
        MISC_HILOGE("observer is null");
        return observer;
    }
    observer->SetKey(key);
    observer->SetUpdateFunc(func);
    return observer;
}

void VibrationPriorityManager::Initialize()
{
    auto sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sm == nullptr) {
        MISC_HILOGE("sm cannot be nullptr");
        return;
    }
    auto remoteObj = sm->GetSystemAbility(MISCDEVICE_SERVICE_ABILITY_ID);
    if (remoteObj == nullptr) {
        MISC_HILOGE("GetSystemAbility return nullptr");
        return;
    }
    remoteObj_ = remoteObj;
}

Uri VibrationPriorityManager::AssembleUri(const std::string &key)
{
    Uri uri(SETTING_URI_PROXY + "&key=" + key);
    return uri;
}

std::shared_ptr<DataShare::DataShareHelper> VibrationPriorityManager::CreateDataShareHelper()
{
    if (remoteObj_ == nullptr) {
        MISC_HILOGE("remoteObj_ is nullptr");
        return nullptr;
    }
    auto helper = DataShare::DataShareHelper::Creator(remoteObj_, SETTING_URI_PROXY, SETTINGS_DATA_EXT_URI);
    if (helper == nullptr) {
        MISC_HILOGW("helper is nullptr, uri:%{public}s", SETTING_URI_PROXY.c_str());
        return nullptr;
    }
    return helper;
}

bool VibrationPriorityManager::ReleaseDataShareHelper(std::shared_ptr<DataShare::DataShareHelper> &helper)
{
    if (!helper->Release()) {
        MISC_HILOGW("release helper fail");
        return false;
    }
    return true;
}

void VibrationPriorityManager::ExecRegisterCb(const sptr<MiscDeviceObserver> &observer)
{
    if (observer == nullptr) {
        MISC_HILOGE("observer is nullptr");
        return;
    }
    observer->OnChange();
}

int32_t VibrationPriorityManager::RegisterObserver(const sptr<MiscDeviceObserver> &observer)
{
    std::string callingIdentity = IPCSkeleton::ResetCallingIdentity();
    auto uri = AssembleUri(observer->GetKey());
    auto helper = CreateDataShareHelper();
    if (helper == nullptr) {
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return MISC_NO_INIT_ERR;
    }
    helper->RegisterObserver(uri, observer);
    helper->NotifyChange(uri);
    std::thread execCb(VibrationPriorityManager::ExecRegisterCb, observer);
    execCb.detach();
    ReleaseDataShareHelper(helper);
    IPCSkeleton::SetCallingIdentity(callingIdentity);
    MISC_HILOGD("succeed to register observer of uri:%{public}s", uri.ToString().c_str());
    return ERR_OK;
}

int32_t VibrationPriorityManager::UnregisterObserver(const sptr<MiscDeviceObserver> &observer)
{
    std::string callingIdentity = IPCSkeleton::ResetCallingIdentity();
    auto uri = AssembleUri(observer->GetKey());
    auto helper = CreateDataShareHelper();
    if (helper == nullptr) {
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return MISC_NO_INIT_ERR;
    }
    helper->UnregisterObserver(uri, observer);
    ReleaseDataShareHelper(helper);
    IPCSkeleton::SetCallingIdentity(callingIdentity);
    MISC_HILOGD("succeed to unregister observer of uri:%{public}s", uri.ToString().c_str());
    return ERR_OK;
}
}  // namespace Sensors
}  // namespace OHOS