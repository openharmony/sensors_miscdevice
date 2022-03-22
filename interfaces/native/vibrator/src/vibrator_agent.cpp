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
#include "vibrator_agent.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"
#include "vibrator_service_client.h"

using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;
using OHOS::Sensors::VibratorServiceClient;

static const HiLogLabel LABEL = { LOG_CORE, OHOS::SensorsLogDomain::MISCDEVICE_VIBRATOR_INTERFACE, "VibratorNDK" };
static const int32_t DEFAULT_VIBRATOR_ID = 123;
static int32_t g_loopingFlag = 0;

void EnableLooping()
{
    g_loopingFlag = 1;
}

int32_t DisableLooping()
{
    g_loopingFlag = 0;
    return StopVibrator(VIBRATOR_STOP_MODE_PRESET);
}

int32_t StartVibrator(const char *effectId)
{
    if (effectId == NULL) {
        HiLog::Error(LABEL, "%{public}s effectId is null", __func__);
        return OHOS::Sensors::ERROR;
    }
    bool isLooping = (g_loopingFlag == 1) ? true : false;
    auto &client = VibratorServiceClient::GetInstance();
    int32_t ret = client.Vibrate(DEFAULT_VIBRATOR_ID, effectId, isLooping);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s vibrator effectId failed, ret: %{public}d", __func__, ret);
        return OHOS::Sensors::ERROR;
    }
    return OHOS::Sensors::SUCCESS;
}

int32_t StartVibratorOnce(uint32_t duration)
{
    if (duration == 0) {
        HiLog::Error(LABEL, "%{public}s duration is invalid", __func__);
        return OHOS::Sensors::ERROR;
    }
    auto &client = VibratorServiceClient::GetInstance();
    int32_t ret = client.Vibrate(DEFAULT_VIBRATOR_ID, duration);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s vibrator duration failed, ret: %{public}d", __func__, ret);
        return OHOS::Sensors::ERROR;
    }
    return OHOS::Sensors::SUCCESS;
}

int32_t StopVibrator(const char *mode)
{
    if (mode == nullptr) {
        HiLog::Error(LABEL, "%{public}s mode is invalid", __func__);
        return OHOS::Sensors::ERROR;
    }
    if (strcmp(mode, "time") != 0 && strcmp(mode, "preset") != 0) {
        HiLog::Error(LABEL, "%{public}s mode is invalid, mode is %{public}s", __func__, mode);
        return OHOS::Sensors::ERROR;
    }
    auto &client = VibratorServiceClient::GetInstance();
    int32_t ret = client.Stop(DEFAULT_VIBRATOR_ID, mode);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s is failed, ret: %{public}d", __func__, ret);
        return OHOS::Sensors::ERROR;
    }
    return OHOS::Sensors::SUCCESS;
}