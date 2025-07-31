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
constexpr size_t U32_AT_SIZE = 12;
VibratorCurvePoint vibratorCurvePoint{
    .time = 0,
    .intensity = 100,
    .frequency = 100
};

VibratorEvent vibratorEvent{
    .type = EVENT_TYPE_CONTINUOUS,
    .time = 0,
    .duration = 500,
    .intensity = 100,
    .frequency = 100,
    .index = 0,
    .pointNum = 1,
    .points = &vibratorCurvePoint
};

VibratorPattern vibratorPattern{
    .time = 500,
    .eventNum = 10,
    .patternDuration = 5000,
    .events = &vibratorEvent
};

VibratorPackage beforeModulationPackage {
    .patternNum = 1,
    .packageDuration = 5000,
    .patterns = &vibratorPattern
};
} // namespace

template<class T>
size_t GetObject(const uint8_t *data, size_t size, T &object)
{
    size_t objectSize = sizeof(object);
    if (objectSize > size) {
        return 0;
    }
    errno_t ret = memcpy_s(&object, objectSize, data, objectSize);
    if (ret != EOK) {
        return 0;
    }
    return objectSize;
}

bool ModulatePackageFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < U32_AT_SIZE) {
        return false;
    }
    VibratorCurvePoint modulationCurve {
        .time = 100,
        .intensity = 0,
        .frequency = 0
    };
    size_t startPos = 0;
    startPos += GetObject<int32_t>(data + startPos, size - startPos, modulationCurve.intensity);
    GetObject<int32_t>(data + startPos, size - startPos, modulationCurve.frequency);
    int32_t curvePointNum = 1;
    int32_t duration = 300;
    VibratorPackage afterModulationPackage;
    OHOS::Sensors::ModulatePackage(&modulationCurve, curvePointNum, duration,
        beforeModulationPackage, afterModulationPackage);
    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::ModulatePackageFuzzTest(data, size);
    return 0;
}