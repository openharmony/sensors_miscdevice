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
#include "vibrator_agent.h"

#include "parameters.h"

#include "sensors_errors.h"
#include "vibrator_service_client.h"

namespace OHOS {
namespace Sensors {
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;
using OHOS::Sensors::VibratorServiceClient;

static const HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "VibratorNDK" };
static const int32_t DEFAULT_VIBRATOR_ID = 123;
static int32_t g_loopCount = 1;
static int32_t g_usage = USAGE_UNKNOWN;
const std::string PHONE_TYPE = "phone";

static int32_t NormalizeErrCode(int32_t code)
{
    switch (code) {
        case PERMISSION_DENIED: {
            return PERMISSION_DENIED;
        }
        case PARAMETER_ERROR: {
            return PARAMETER_ERROR;
        }
        case IS_NOT_SUPPORTED: {
            return IS_NOT_SUPPORTED;
        }
        default: {
            MISC_HILOGW("Operating the device fail");
            return DEVICE_OPERATION_FAILED;
        }
    }
}

bool SetLoopCount(int32_t count)
{
    if (count <= 0) {
        MISC_HILOGE("Input invalid, count is %{public}d", count);
        return false;
    }
    g_loopCount = count;
    return true;
}

int32_t StartVibrator(const char *effectId)
{
    CHKPR(effectId, PARAMETER_ERROR);
    auto &client = VibratorServiceClient::GetInstance();
    int32_t ret = client.Vibrate(DEFAULT_VIBRATOR_ID, effectId, g_loopCount, g_usage);
    if (ret != ERR_OK) {
        MISC_HILOGE("Vibrate effectId failed, ret:%{public}d", ret);
        return NormalizeErrCode(ret);
    }
    g_loopCount = 1;
    g_usage = USAGE_UNKNOWN;
    return SUCCESS;
}

int32_t StartVibratorOnce(int32_t duration)
{
    if (duration <= 0) {
        MISC_HILOGE("duration is invalid");
        return PARAMETER_ERROR;
    }
    auto &client = VibratorServiceClient::GetInstance();
    int32_t ret = client.Vibrate(DEFAULT_VIBRATOR_ID, duration, g_usage);
    if (ret != ERR_OK) {
        MISC_HILOGE("Vibrate duration failed, ret:%{public}d", ret);
        return NormalizeErrCode(ret);
    }
    g_usage = USAGE_UNKNOWN;
    return SUCCESS;
}

bool IsSupportVibratorCustom()
{
    return (OHOS::system::GetDeviceType() == PHONE_TYPE);
}

int32_t PlayVibratorCustom(int32_t fd, int64_t offset, int64_t length)
{
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    if (fd < 0 || offset < 0 || length <= 0) {
        MISC_HILOGE("Input parameter invalid, fd:%{public}d, offset:%{public}lld, length:%{public}lld",
            fd, static_cast<long long>(offset), static_cast<long long>(length));
        return PARAMETER_ERROR;
    }
    auto &client = VibratorServiceClient::GetInstance();
    RawFileDescriptor rawFd = {
        .fd = fd,
        .offset = offset,
        .length = length
    };
    int32_t ret = client.PlayVibratorCustom(DEFAULT_VIBRATOR_ID, rawFd, g_usage);
    if (ret != ERR_OK) {
        MISC_HILOGE("PlayVibratorCustom failed, ret:%{public}d", ret);
        return NormalizeErrCode(ret);
    }
    g_usage = USAGE_UNKNOWN;
    return SUCCESS;
#else
    MISC_HILOGE("The device does not support this operation");
    return IS_NOT_SUPPORTED;
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
}

int32_t StopVibrator(const char *mode)
{
    CHKPR(mode, PARAMETER_ERROR);
    if (strcmp(mode, "time") != 0 && strcmp(mode, "preset") != 0) {
        MISC_HILOGE("Input parameter invalid, mode is %{public}s", mode);
        return PARAMETER_ERROR;
    }
    auto &client = VibratorServiceClient::GetInstance();
    int32_t ret = client.StopVibrator(DEFAULT_VIBRATOR_ID, mode);
    if (ret != ERR_OK) {
        MISC_HILOGE("StopVibrator failed, ret:%{public}d", ret);
        return NormalizeErrCode(ret);
    }
    return SUCCESS;
}

int32_t Cancel()
{
    auto &client = VibratorServiceClient::GetInstance();
    int32_t ret = client.StopVibrator(DEFAULT_VIBRATOR_ID);
    if (ret != ERR_OK) {
        MISC_HILOGE("StopVibrator failed, ret:%{public}d", ret);
        return NormalizeErrCode(ret);
    }
    return SUCCESS;
}

bool SetUsage(int32_t usage)
{
    if ((usage < 0) || (usage >= USAGE_MAX)) {
        MISC_HILOGE("Input invalid, usage is %{public}d", usage);
        return false;
    }
    g_usage = usage;
    return true;
}

int32_t IsSupportEffect(const char *effectId, bool *state)
{
    CHKPR(effectId, PARAMETER_ERROR);
    auto &client = VibratorServiceClient::GetInstance();
    int32_t ret = client.IsSupportEffect(effectId, *state);
    if (ret != ERR_OK) {
        MISC_HILOGE("Query effect support failed, ret:%{public}d", ret);
        return NormalizeErrCode(ret);
    }
    return SUCCESS;
}
}  // namespace Sensors
}  // namespace OHOS