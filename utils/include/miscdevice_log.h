/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#ifndef MISCDEVICE_LOG_H
#define MISCDEVICE_LOG_H

#include "hilog/log.h"

#include "sensors_errors.h"
#include "miscdevice_log.h"


namespace OHOS {
namespace Sensors {
namespace {
constexpr uint32_t MISC_LOG_DOMAIN = 0xD002800;
} // namespace
#ifndef MISC_FUNC_FMT
#define MISC_FUNC_FMT "in %{public}s, #%{public}d, "
#endif

#ifndef MISC_FUNC_INFO
#define MISC_FUNC_INFO __FUNCTION__, __LINE__
#endif

#define MISC_HILOGD(fmt, ...) do { \
    OHOS::HiviewDFX::HiLog::Debug(LABEL, MISC_FUNC_FMT fmt, MISC_FUNC_INFO, ##__VA_ARGS__); \
} while (0)
#define MISC_HILOGI(fmt, ...) do { \
    OHOS::HiviewDFX::HiLog::Info(LABEL, MISC_FUNC_FMT fmt, MISC_FUNC_INFO, ##__VA_ARGS__); \
} while (0)
#define MISC_HILOGW(fmt, ...) do { \
    OHOS::HiviewDFX::HiLog::Warn(LABEL, MISC_FUNC_FMT fmt, MISC_FUNC_INFO, ##__VA_ARGS__); \
} while (0)
#define MISC_HILOGE(fmt, ...) do { \
    OHOS::HiviewDFX::HiLog::Error(LABEL, MISC_FUNC_FMT fmt, MISC_FUNC_INFO, ##__VA_ARGS__); \
} while (0)
#define MISC_LOGF(fmt, ...) do { \
    OHOS::HiviewDFX::HiLog::Fatal(LABEL, MISC_FUNC_FMT fmt, MISC_FUNC_INFO, ##__VA_ARGS__); \
} while (0)
} // namespace Sensors
} // namespace OHOS
#endif // MISCDEVICE_LOG_H