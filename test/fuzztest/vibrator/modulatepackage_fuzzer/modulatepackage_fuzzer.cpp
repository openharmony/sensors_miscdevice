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

#include "modulatepackage_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fuzzer/FuzzedDataProvider.h>
#include <string>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "securec.h"
#include "token_setproc.h"

#include "vibrator_agent.h"

namespace OHOS {
using namespace Security::AccessToken;
using Security::AccessToken::AccessTokenID;

namespace {
constexpr int32_t PARAMETER_NUM_ONE = 1;
} // namespace

bool ModulatePackageFuzzTest(FuzzedDataProvider &provider)
{
    VibratorCurvePoint modulationCurve;
    modulationCurve.time = provider.ConsumeIntegral<int32_t>();
    modulationCurve.intensity = provider.ConsumeIntegral<int32_t>();
    modulationCurve.frequency = provider.ConsumeIntegral<int32_t>();
    VibratorEvent event;
    event.time = provider.ConsumeIntegral<int32_t>();
    event.duration = provider.ConsumeIntegral<int32_t>();
    event.intensity = provider.ConsumeIntegral<int32_t>();
    event.frequency = provider.ConsumeIntegral<int32_t>();
    event.pointNum = PARAMETER_NUM_ONE;
    event.points = &modulationCurve;
    VibratorPattern pattern;
    pattern.time = provider.ConsumeIntegral<int32_t>();
    pattern.eventNum = PARAMETER_NUM_ONE;
    pattern.patternDuration = provider.ConsumeIntegral<int32_t>();
    pattern.events = &event;
    int32_t curvePointNum = 1;
    int32_t duration = 300;
    VibratorPackage package;
    package.patternNum = PARAMETER_NUM_ONE;
    package.packageDuration = provider.ConsumeIntegral<int32_t>();
    package.patterns = &pattern;
    VibratorPackage packageAfterModulation;
    packageAfterModulation.patternNum = PARAMETER_NUM_ONE;
    packageAfterModulation.packageDuration = provider.ConsumeIntegral<int32_t>();
    packageAfterModulation.patterns = &pattern;
    OHOS::Sensors::ModulatePackage(&modulationCurve, curvePointNum, duration, package, packageAfterModulation);
    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::ModulatePackageFuzzTest(provider);
    return 0;
}