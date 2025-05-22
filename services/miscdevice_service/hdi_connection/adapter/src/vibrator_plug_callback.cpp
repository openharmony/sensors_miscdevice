/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <set>
#include "hdi_connection.h"
#include "vibrator_plug_callback.h"
#include "sensors_errors.h"

#undef LOG_TAG
#define LOG_TAG "VibratorPlugCallback"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
std::unique_ptr<HdiConnection> HdiConnection_ = std::make_unique<HdiConnection>();
} // namespace

int32_t VibratorPlugCallback::OnVibratorPlugEvent(const HdfVibratorPlugInfo &vibratorPlugInfo)
{
    DevicePlugCallback devicePlugCb_ = HdiConnection_->GetVibratorPlugCb();
    CHKPR(devicePlugCb_, ERR_NO_INIT);
    devicePlugCb_(vibratorPlugInfo);
    return ERR_OK;
}
} // namespace Sensors
} // namespace OHOS