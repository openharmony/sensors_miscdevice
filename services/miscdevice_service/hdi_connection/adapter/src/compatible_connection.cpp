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
#include "compatible_connection.h"

#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>

#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "CompatibleConnection" };
std::unordered_map<std::string, int32_t> vibratorEffect_ = {
    {"haptic.clock.timer", 2000},
    {"haptic.default.effect", 804}
};
HdfVibratorMode vibrateMode_;
}
uint32_t CompatibleConnection::duration_ = 0;
std::atomic_bool CompatibleConnection::isStop_ = false;

int32_t CompatibleConnection::ConnectHdi()
{
    CALL_LOG_ENTER;
    return ERR_OK;
}

int32_t CompatibleConnection::StartOnce(uint32_t duration)
{
    CALL_LOG_ENTER;
    duration_ = duration;
    if (!vibrateThread_.joinable()) {
        std::thread senocdDataThread(CompatibleConnection::VibrateProcess);
        vibrateThread_ = std::move(senocdDataThread);
        isStop_ = false;
    }
    vibrateMode_ = HDF_VIBRATOR_MODE_ONCE;
    return ERR_OK;
}

int32_t CompatibleConnection::Start(const std::string &effectType)
{
    CALL_LOG_ENTER;
    if (vibratorEffect_.find(effectType) == vibratorEffect_.end()) {
        MISC_HILOGE("Do not support effectType:%{public}s", effectType.c_str());
        return VIBRATOR_ON_ERR;
    }
    duration_ = vibratorEffect_[effectType];
    if (!vibrateThread_.joinable()) {
        std::thread senocdDataThread(CompatibleConnection::VibrateProcess);
        vibrateThread_ = std::move(senocdDataThread);
        isStop_ = false;
    }
    vibrateMode_ = HDF_VIBRATOR_MODE_PRESET;
    return ERR_OK;
}

#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
int32_t CompatibleConnection::EnableCompositeEffect(const HdfCompositeEffect &hdfCompositeEffect)
{
    CALL_LOG_ENTER;
    if (hdfCompositeEffect.compositeEffects.empty()) {
        MISC_HILOGE("compositeEffects is empty");
        return VIBRATOR_ON_ERR;
    }
    duration_ = 0;
    size_t size = hdfCompositeEffect.compositeEffects.size();
    for (size_t i = 0; i < size; ++i) {
        if (hdfCompositeEffect.type == HDF_EFFECT_TYPE_TIME) {
            duration_ += hdfCompositeEffect.compositeEffects[i].timeEffect.delay;
        } else if (hdfCompositeEffect.type == HDF_EFFECT_TYPE_PRIMITIVE) {
            duration_ += hdfCompositeEffect.compositeEffects[i].primitiveEffect.delay;
        }
    }
    if (!vibrateThread_.joinable()) {
        std::thread senocdDataThread(CompatibleConnection::VibrateProcess);
        vibrateThread_ = std::move(senocdDataThread);
        isStop_ = false;
    }
    vibrateMode_ = HDF_VIBRATOR_MODE_PRESET;
    return ERR_OK;
}

bool CompatibleConnection::IsVibratorRunning()
{
    CALL_LOG_ENTER;
    return (!isStop_);
}

std::optional<HdfEffectInfo> CompatibleConnection::GetEffectInfo(const std::string &effect)
{
    CALL_LOG_ENTER;
    HdfEffectInfo effectInfo;
    if (vibratorEffect_.find(effect) == vibratorEffect_.end()) {
        MISC_HILOGI("Not support effect:%{public}s", effect.c_str());
        effectInfo.isSupportEffect = false;
        effectInfo.duration = 0;
        return effectInfo;
    }
    effectInfo.isSupportEffect = true;
    effectInfo.duration = vibratorEffect_[effect];
    return effectInfo;
}
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM

int32_t CompatibleConnection::Stop(HdfVibratorMode mode)
{
    CALL_LOG_ENTER;
    if (mode < 0 || mode >= HDF_VIBRATOR_MODE_BUTT) {
        MISC_HILOGE("invalid mode:%{public}d", mode);
        return VIBRATOR_OFF_ERR;
    }
    if (vibrateMode_ != mode) {
        MISC_HILOGE("should start vibrate first");
        return VIBRATOR_OFF_ERR;
    }
    if (vibrateThread_.joinable()) {
        MISC_HILOGI("stop vibrate thread");
        isStop_ = true;
        vibrateThread_.join();
    }
    return ERR_OK;
}

int32_t CompatibleConnection::DestroyHdiConnection()
{
    CALL_LOG_ENTER;
    return ERR_OK;
}

void CompatibleConnection::VibrateProcess()
{
    CALL_LOG_ENTER;
    clock_t vibrateStartTime = clock();
    while (static_cast<uint32_t>(clock() - vibrateStartTime) < duration_) {
        if (isStop_) {
            MISC_HILOGI("thread should stop");
            break;
        }
    }
    isStop_ = true;
    return;
}
}  // namespace Sensors
}  // namespace OHOS
