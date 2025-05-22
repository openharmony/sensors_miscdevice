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

 #ifndef VIBRATOR_PLUG_CALLBACK_H
 #define VIBRATOR_PLUG_CALLBACK_H
 
 #include <nocopyable.h>
 #ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
 #include "v2_0/ivibrator_interface.h"
 #include "v2_0/ivibrator_plug_callback.h"
 #include "v2_0/vibrator_types.h"
 #endif // HDF_DRIVERS_INTERFACE_VIBRATOR
 
 #ifdef HDF_DRIVERS_INTERFACE_VIBRATOR
 using OHOS::HDI::Vibrator::V2_0::IVibratorPlugCallback;
 #endif // HDF_DRIVERS_INTERFACE_VIBRATOR
 
 namespace OHOS {
 namespace Sensors {
 class VibratorPlugCallback : public IVibratorPlugCallback {
 public:
     virtual ~VibratorPlugCallback() {}
     int32_t OnVibratorPlugEvent(const HdfVibratorPlugInfo &vibratorPlugInfo) override;
 };
 } // namespace Sensors
 } // namespace OHOS
 #endif // VIBRATOR_PLUG_CALLBACK_H
 