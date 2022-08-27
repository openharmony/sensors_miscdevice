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

#include "miscdevice_service.h"

#include <string_ex.h>

#include "sensors_errors.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "MiscdeviceService" };
constexpr int32_t MIN_VIBRATOR_TIME = 0;
constexpr int32_t MAX_VIBRATOR_TIME = 1800000;
constexpr int32_t DEFAULT_VIBRATOR_ID = 123;
}  // namespace

bool MiscdeviceService::ready_ = false;
std::mutex MiscdeviceService::conditionVarMutex_;
std::condition_variable MiscdeviceService::conditionVar_;
std::unordered_map<std::string, int32_t> MiscdeviceService::hapticRingMap_ = {
    {"haptic.ringtone.Bounce", 6762},
    {"haptic.ringtone.Cartoon", 2118},
    {"haptic.ringtone.Chilled", 11947},
    {"haptic.ringtone.Classic_Bell", 5166},
    {"haptic.ringtone.Concentrate", 15707},
    {"haptic.ringtone.Day_lily", 5695},
    {"haptic.ringtone.Digital_Ringtone", 3756},
    {"haptic.ringtone.Dream", 6044},
    {"haptic.ringtone.Dream_It_Possible", 39710},
    {"haptic.ringtone.Dynamo", 15148},
    {"haptic.ringtone.Flipped", 20477},
    {"haptic.ringtone.Forest_Day", 6112},
    {"haptic.ringtone.Free", 9917},
    {"haptic.ringtone.Halo", 16042},
    {"haptic.ringtone.Harp", 10030},
    {"haptic.ringtone.Hello_Ya", 35051},
    {"haptic.ringtone.Menuet", 8261},
    {"haptic.ringtone.Neon", 23925},
    {"haptic.ringtone.Notes", 9051},
    {"haptic.ringtone.Pulse", 27550},
    {"haptic.ringtone.Sailing", 19188},
    {"haptic.ringtone.Sax", 4780},
    {"haptic.ringtone.Spin", 6000},
    {"haptic.ringtone.Tune_Clean", 13342},
    {"haptic.ringtone.Tune_Living", 17249},
    {"haptic.ringtone.Tune_Orchestral", 15815},
    {"haptic.ringtone.Westlake", 11654},
    {"haptic.ringtone.Whistle", 20276},
    {"haptic.ringtone.Amusement_Park", 9441},
    {"haptic.ringtone.Breathe_Freely", 30887},
    {"haptic.ringtone.Summer_Afternoon", 31468},
    {"haptic.ringtone.Surging_Power", 17125},
    {"haptic.ringtone.Sunlit_Garden", 38330},
    {"haptic.ringtone.Fantasy_World", 15301}
};

REGISTER_SYSTEM_ABILITY_BY_ID(MiscdeviceService, MISCDEVICE_SERVICE_ABILITY_ID, true);

MiscdeviceService::MiscdeviceService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate),
      lightExist_(false),
      vibratorExist_(false),
      state_(MiscdeviceServiceState::STATE_STOPPED),
      vibratorThread_(nullptr)
{}

MiscdeviceService::~MiscdeviceService()
{
    if (vibratorThread_ != nullptr) {
        while (vibratorThread_->IsRunning()) {
            vibratorThread_->NotifyExit();
            vibratorThread_->NotifyExitSync();
        }
        delete vibratorThread_;
        vibratorThread_ = nullptr;
    }
}

void MiscdeviceService::OnDump()
{
    MISC_HILOGI("ondump is invoked");
}

void MiscdeviceService::OnStart()
{
    CALL_LOG_ENTER;
    if (state_ == MiscdeviceServiceState::STATE_RUNNING) {
        MISC_HILOGW("state_ already started");
        return;
    }
    if (!InitInterface()) {
        MISC_HILOGE("Init interface error");
        return;
    }
    if (!SystemAbility::Publish(this)) {
        MISC_HILOGE("publish MiscdeviceService failed");
        return;
    }
    auto ret = miscDdeviceIdMap_.insert(std::make_pair(MiscdeviceDeviceId::LED, lightExist_));
    if (!ret.second) {
        MISC_HILOGI("light exist in miscDdeviceIdMap_");
        ret.first->second = lightExist_;
    }
    ret = miscDdeviceIdMap_.insert(std::make_pair(MiscdeviceDeviceId::VIBRATOR, vibratorExist_));
    if (!ret.second) {
        MISC_HILOGI("vibrator exist in miscDdeviceIdMap_");
        ret.first->second = vibratorExist_;
    }
    state_ = MiscdeviceServiceState::STATE_RUNNING;
}

bool MiscdeviceService::InitInterface()
{
    auto ret = vibratorHdiConnection_.ConnectHdi();
    if (ret != ERR_OK) {
        MISC_HILOGE("InitVibratorServiceImpl failed");
        return false;
    }
    return true;
}

void MiscdeviceService::OnStop()
{
    CALL_LOG_ENTER;
    if (state_ == MiscdeviceServiceState::STATE_STOPPED) {
        MISC_HILOGW("MiscdeviceService stopped already");
        return;
    }
    state_ = MiscdeviceServiceState::STATE_STOPPED;
    int32_t ret = vibratorHdiConnection_.DestroyHdiConnection();
    if (ret != ERR_OK) {
        MISC_HILOGE("destroy hdi connection fail");
    }
}

bool MiscdeviceService::IsAbilityAvailable(MiscdeviceDeviceId groupID)
{
    auto it = miscDdeviceIdMap_.find(groupID);
    if (it == miscDdeviceIdMap_.end()) {
        MISC_HILOGE("cannot find groupID : %{public}d", groupID);
        return false;
    }
    return it->second;
}

bool MiscdeviceService::IsVibratorEffectAvailable(int32_t vibratorId, const std::string &effectType)
{
    return true;
}

std::vector<int32_t> MiscdeviceService::GetVibratorIdList()
{
    std::vector<int32_t> vibratorIds = { DEFAULT_VIBRATOR_ID };
    return vibratorIds;
}

int32_t MiscdeviceService::Vibrate(int32_t vibratorId, uint32_t timeOut)
{
    if ((timeOut < MIN_VIBRATOR_TIME) || (timeOut > MAX_VIBRATOR_TIME)) {
        MISC_HILOGE("timeOut is invalid, timeOut : %{public}u", timeOut);
        return ERR_INVALID_VALUE;
    }
    std::lock_guard<std::mutex> vibratorEffectLock(vibratorEffectMutex_);
    auto it = vibratorEffectMap_.find(vibratorId);
    if (it != vibratorEffectMap_.end()) {
        if (it->second == "time") {
            vibratorHdiConnection_.Stop(IVibratorHdiConnection::VIBRATOR_STOP_MODE_TIME);
        } else {
            vibratorHdiConnection_.Stop(IVibratorHdiConnection::VIBRATOR_STOP_MODE_PRESET);
        }
    }
    vibratorEffectMap_[vibratorId] = "time";
    int32_t ret = vibratorHdiConnection_.StartOnce((timeOut < MIN_VIBRATOR_TIME) ? MIN_VIBRATOR_TIME : timeOut);
    if (ret != ERR_OK) {
        MISC_HILOGE("Vibrate failed, error: %{public}d", ret);
        return ERROR;
    }
    miscdeviceDump_.SaveVibrator(GetCallingTokenID(), GetCallingUid(), GetCallingPid(), timeOut);
    return ret;
}

int32_t MiscdeviceService::CancelVibrator(int32_t vibratorId)
{
    std::lock_guard<std::mutex> vibratorEffectLock(vibratorEffectMutex_);
    auto it = vibratorEffectMap_.find(vibratorId);
    if (it != vibratorEffectMap_.end() && it->second == "time") {
        MISC_HILOGI("stop mode is %{public}s", it->second.c_str());
        vibratorEffectMap_.clear();
    } else {
        MISC_HILOGE("vibratorEffectMap_ is failed");
        return ERROR;
    }
    if (vibratorThread_ != nullptr) {
        while (vibratorThread_->IsRunning()) {
            MISC_HILOGI("stop previous vibratorThread, vibratorId : %{public}d", vibratorId);
            vibratorThread_->NotifyExit();
            vibratorThread_->NotifyExitSync();
        }
    }
    return vibratorHdiConnection_.Stop(IVibratorHdiConnection::VIBRATOR_STOP_MODE_TIME);
}

int32_t MiscdeviceService::PlayVibratorEffect(int32_t vibratorId, const std::string &effect, bool isLooping)
{
    std::lock_guard<std::mutex> vibratorEffectLock(vibratorEffectMutex_);
    auto it = vibratorEffectMap_.find(vibratorId);
    if (it != vibratorEffectMap_.end()) {
        if (it->second == "time") {
            if ((vibratorThread_ != nullptr) && (vibratorThread_->IsRunning())) {
                MISC_HILOGI("stop previous vibratorThread");
                vibratorThread_->NotifyExit();
                vibratorThread_->NotifyExitSync();
            }
            vibratorHdiConnection_.Stop(IVibratorHdiConnection::VIBRATOR_STOP_MODE_TIME);
        } else {
            vibratorHdiConnection_.Stop(IVibratorHdiConnection::VIBRATOR_STOP_MODE_PRESET);
        }
    }
    if (!isLooping) {
        vibratorEffectMap_[vibratorId] = effect;
        int32_t ret = vibratorHdiConnection_.Start(effect);
        if (ret != ERR_OK) {
            MISC_HILOGE("PlayVibratorEffect failed, error: %{public}d", ret);
            return ERROR;
        }
        miscdeviceDump_.SaveVibratorEffect(GetCallingTokenID(), GetCallingUid(), GetCallingPid(), effect);
        return ret;
    }
    std::unordered_map<std::string, int32_t>::iterator iter = hapticRingMap_.find(effect);
    if (iter == hapticRingMap_.end()) {
        MISC_HILOGE("hapticRingMap_ is not exist");
        return ERROR;
    }
    vibratorEffectMap_[vibratorId] = effect;
    int32_t delayTiming = iter->second;
    if (vibratorEffectThread_ == nullptr) {
        std::lock_guard<std::mutex> lock(vibratorEffectThreadMutex_);
        if (vibratorEffectThread_ == nullptr) {
            vibratorEffectThread_ = std::make_unique<VibratorEffectThread>(*this);
            if (vibratorEffectThread_ == nullptr) {
                MISC_HILOGE("vibratorEffectThread_ cannot be null");
                return ERROR;
            }
        }
    }
    while (vibratorEffectThread_->IsRunning()) {
        MISC_HILOGD("notify the vibratorEffectThread");
        ready_ = true;
        conditionVar_.notify_one();
        ready_ = false;
    }
    vibratorEffectThread_->UpdateVibratorEffectData(effect, delayTiming);
    vibratorEffectThread_->Start("VibratorEffectThread");
    return NO_ERROR;
}

int32_t MiscdeviceService::PlayCustomVibratorEffect(int32_t vibratorId, const std::vector<int32_t> &timing,
                                                    const std::vector<int32_t> &intensity, int32_t periodCount)
{
    if (!MiscdeviceCommon::CheckCustomVibratorEffect(timing, intensity, periodCount)) {
        MISC_HILOGE("params are invalid");
        return ERR_INVALID_VALUE;
    }
    std::lock_guard<std::mutex> vibratorEffectLock(vibratorEffectMutex_);
    auto it = vibratorEffectMap_.find(vibratorId);
    if (it != vibratorEffectMap_.end()) {
        if (it->second == "time") {
            vibratorHdiConnection_.Stop(IVibratorHdiConnection::VIBRATOR_STOP_MODE_TIME);
        } else {
            vibratorHdiConnection_.Stop(IVibratorHdiConnection::VIBRATOR_STOP_MODE_PRESET);
        }
    }
    vibratorEffectMap_[vibratorId] = "time";
    if (vibratorThread_ == nullptr) {
        std::lock_guard<std::mutex> lock(vibratorThreadMutex_);
        if (vibratorThread_ == nullptr) {
            vibratorThread_ = new (std::nothrow) VibratorThread(*this);
            if (vibratorThread_ == nullptr) {
                MISC_HILOGE("vibratorThread_ cannot be null");
                return ERROR;
            }
        }
    }
    // check current vibrator execution sequences and abort
    while (vibratorThread_->IsRunning()) {
        MISC_HILOGI("stop previous vibratorThread, vibratorId : %{public}d", vibratorId);
        vibratorThread_->NotifyExit();
        vibratorThread_->NotifyExitSync();
    }
    MISC_HILOGI("update vibrator data and start, vibratorId : %{public}d", vibratorId);
    vibratorThread_->UpdateVibratorData(timing, intensity, periodCount);
    vibratorThread_->Start("VibratorThread");
    return NO_ERROR;
}

int32_t MiscdeviceService::StopVibratorEffect(int32_t vibratorId, const std::string &effect)
{
    std::string curEffect = effect;
    std::lock_guard<std::mutex> vibratorEffectLock(vibratorEffectMutex_);
    auto it = vibratorEffectMap_.find(vibratorId);
    if ((it != vibratorEffectMap_.end()) && (it->second != "time")) {
        MISC_HILOGI("vibrator effect is %{public}s", it->second.c_str());
        curEffect = it->second;
        vibratorEffectMap_.clear();
    } else {
        MISC_HILOGE("vibrator effect is failed");
        return ERROR;
    }
    MISC_HILOGI("curEffect : %{public}s", curEffect.c_str());
    if (vibratorEffectThread_ != nullptr) {
        while (vibratorEffectThread_->IsRunning()) {
            MISC_HILOGD("notify the vibratorEffectThread, vibratorId : %{public}d", vibratorId);
            ready_ = true;
            conditionVar_.notify_one();
            ready_ = false;
        }
    }
    int32_t ret = vibratorHdiConnection_.Stop(IVibratorHdiConnection::VIBRATOR_STOP_MODE_PRESET);
    return ret;
}

int32_t MiscdeviceService::SetVibratorParameter(int32_t vibratorId, const std::string &cmd)
{
    return 0;
}

std::string MiscdeviceService::GetVibratorParameter(int32_t vibratorId, const std::string &cmd)
{
    return cmd;
}

std::vector<int32_t> MiscdeviceService::GetLightSupportId()
{
    std::vector<int32_t> list;
    return list;
}

bool MiscdeviceService::IsLightEffectSupport(int32_t lightId, const std::string &effectId)
{
    return false;
}

int32_t MiscdeviceService::Light(int32_t lightId, uint64_t brightness, uint32_t timeOn, uint32_t timeOff)
{
    return 0;
}

int32_t MiscdeviceService::PlayLightEffect(int32_t lightId, const std::string &type)
{
    return 0;
}

int32_t MiscdeviceService::StopLightEffect(int32_t lightId)
{
    return 0;
}

MiscdeviceService::VibratorThread::VibratorThread(const MiscdeviceService &service)
    : periodCount_(0)
{
    mtx_.lock();
}

void MiscdeviceService::VibratorThread::NotifyExit()
{
    mtx_.unlock();
}

bool MiscdeviceService::VibratorThread::Run()
{
    return false;
}

void MiscdeviceService::VibratorThread::UpdateVibratorData(const std::vector<int32_t> &timing,
                                                           const std::vector<int32_t> &intensity, int32_t &periodCount)
{
    vecTimingMs_ = timing;
    intensitys_ = intensity;
    periodCount_ = periodCount;
}

MiscdeviceService::VibratorEffectThread::VibratorEffectThread(const MiscdeviceService &service)
    : delayTimingMs_(0)
{
}

bool MiscdeviceService::VibratorEffectThread::Run()
{
    return false;
}

void MiscdeviceService::VibratorEffectThread::UpdateVibratorEffectData(const std::string effect, int32_t delayTiming)
{
    hapticEffect_ = effect;
    delayTimingMs_ = delayTiming;
}

int32_t MiscdeviceService::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    CALL_LOG_ENTER;
    if (fd < 0) {
        MISC_HILOGE("Invalid fd");
        return DUMP_PARAM_ERR;
    }
    if (args.empty()) {
        MISC_HILOGE("args cannot be empty");
        dprintf(fd, "args cannot be empty\n");
        miscdeviceDump_.DumpHelp(fd);
        return DUMP_PARAM_ERR;
    }
    std::vector<std::string> argList = { "" };
    std::transform(args.begin(), args.end(), std::back_inserter(argList),
        [](const std::u16string &arg) {
        return Str16ToStr8(arg);
    });
    miscdeviceDump_.ParseCommand(fd, argList);
    return ERR_OK;
}
}  // namespace Sensors
}  // namespace OHOS
