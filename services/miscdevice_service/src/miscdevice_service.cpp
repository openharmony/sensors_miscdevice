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
#include "vibration_priority_manager.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "MiscdeviceService" };
constexpr int32_t MIN_VIBRATOR_TIME = 0;
constexpr int32_t MAX_VIBRATOR_TIME = 1800000;
constexpr int32_t DEFAULT_VIBRATOR_ID = 123;
}  // namespace

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
        }
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

bool MiscdeviceService::ShouldIgnoreVibrate(const VibrateInfo &info)
{
    return (PriorityManager->ShouldIgnoreVibrate(info, vibratorThread_) != VIBRATION);
}

int32_t MiscdeviceService::Vibrate(int32_t vibratorId, int32_t timeOut, int32_t usage)
{
    if ((timeOut < MIN_VIBRATOR_TIME) || (timeOut > MAX_VIBRATOR_TIME)
        || (usage >= USAGE_MAX) || (usage < 0)) {
        MISC_HILOGE("Invalid parameter");
        return ERROR;
    }
    VibrateInfo info = {
        .mode = "time",
        .packageName = GetPackageName(GetCallingTokenID()),
        .pid = GetCallingPid(),
        .usage = usage,
        .duration = timeOut
    };
    std::lock_guard<std::mutex> lock(vibratorThreadMutex_);
    if (ShouldIgnoreVibrate(info)) {
        MISC_HILOGE("Vibration is ignored and high priority is vibrating");
        return ERROR;
    }
    StartVibrateThread(info);
    miscdeviceDump_.SaveVibrator(info.packageName, GetCallingUid(), GetCallingPid(), timeOut);
    return NO_ERROR;
}

int32_t MiscdeviceService::CancelVibrator(int32_t vibratorId)
{
    std::lock_guard<std::mutex> lock(vibratorThreadMutex_);
    if ((vibratorThread_ == nullptr) || (!vibratorThread_->IsRunning())) {
        MISC_HILOGE("No vibration, no need to stop");
        return ERROR;
    }
    while (vibratorThread_->IsRunning()) {
        MISC_HILOGD("Notify the vibratorThread, vibratorId : %{public}d", vibratorId);
        vibratorThread_->NotifyExit();
    }
    return NO_ERROR;
}

int32_t MiscdeviceService::PlayVibratorEffect(int32_t vibratorId, const std::string &effect,
    int32_t count, int32_t usage)
{
    if ((vibratorEffects.find(effect) == vibratorEffects.end()) || (count < 1)
        || (usage >= USAGE_MAX) || (usage < 0)) {
        MISC_HILOGE("Invalid parameter");
        return ERROR;
    }
    VibrateInfo info = {
        .mode = "preset",
        .packageName = GetPackageName(GetCallingTokenID()),
        .pid = GetCallingPid(),
        .usage = usage,
        .duration = vibratorEffects[effect],
        .effect = effect,
        .count = count
    };
    std::lock_guard<std::mutex> lock(vibratorThreadMutex_);
    if (ShouldIgnoreVibrate(info)) {
        MISC_HILOGE("Vibration is ignored and high priority is vibrating");
        return ERROR;
    }
    StartVibrateThread(info);
    miscdeviceDump_.SaveVibratorEffect(info.packageName, GetCallingUid(), GetCallingPid(), effect);
    return NO_ERROR;
}

void MiscdeviceService::StartVibrateThread(VibrateInfo info)
{
    if (vibratorThread_ == nullptr) {
        vibratorThread_ = std::make_shared<VibratorThread>();
    }
    while (vibratorThread_->IsRunning()) {
        vibratorThread_->NotifyExit();
    }
    vibratorThread_->UpdateVibratorEffect(info);
    vibratorThread_->Start("VibratorThread");
}

int32_t MiscdeviceService::PlayCustomVibratorEffect(int32_t vibratorId, const std::vector<int32_t> &timing,
                                                    const std::vector<int32_t> &intensity, int32_t periodCount)
{
    if (!MiscdeviceCommon::CheckCustomVibratorEffect(timing, intensity, periodCount)) {
        MISC_HILOGE("params are invalid");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t MiscdeviceService::StopVibratorEffect(int32_t vibratorId, const std::string &effect)
{
    std::lock_guard<std::mutex> lock(vibratorThreadMutex_);
    if ((vibratorThread_ == nullptr) || (!vibratorThread_->IsRunning())) {
        MISC_HILOGE("No vibration, no need to stop");
        return ERROR;
    }
    const VibrateInfo info = vibratorThread_->GetCurrentVibrateInfo();
    if ((info.mode != effect) || (info.pid != GetCallingPid())) {
        MISC_HILOGE("Stop vibration information mismatch");
        return ERROR;
    }
    while (vibratorThread_->IsRunning()) {
        MISC_HILOGD("notify the vibratorThread, vibratorId : %{public}d", vibratorId);
        vibratorThread_->NotifyExit();
    }
    return NO_ERROR;
}

std::string MiscdeviceService::GetPackageName(AccessTokenID tokenId)
{
    std::string packageName;
    int32_t tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    switch (tokenType) {
        case ATokenTypeEnum::TOKEN_HAP: {
            HapTokenInfo hapInfo;
            if (AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo) != 0) {
                MISC_HILOGE("get hap token info fail");
                return {};
            }
            packageName = hapInfo.bundleName;
            break;
        }
        case ATokenTypeEnum::TOKEN_NATIVE:
        case ATokenTypeEnum::TOKEN_SHELL: {
            NativeTokenInfo tokenInfo;
            if (AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo) != 0) {
                MISC_HILOGE("get native token info fail");
                return {};
            }
            packageName = tokenInfo.processName;
            break;
        }
        default: {
            MISC_HILOGW("token type not match");
            break;
        }
    }
    return packageName;
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
