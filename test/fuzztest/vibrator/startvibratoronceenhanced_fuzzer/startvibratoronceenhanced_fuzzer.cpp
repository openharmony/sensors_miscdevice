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

#include "startvibratoronceenhanced_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "securec.h"
#include "vibrator_agent.h"

namespace {
constexpr int32_t MAX_VIBRATOR_TIME = 1800000;
constexpr int32_t MIN_VIBRATOR_TIME = 0;
constexpr size_t U32_AT_SIZE = 12;
}  // namespace

namespace OHOS {
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

bool StartVibratorOnceEnhancedFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < U32_AT_SIZE) {
        return false;
    }
    VibratorIdentifier identifier;
    size_t startPos = 0;
    int32_t duration = 0;
    startPos += GetObject<int32_t>(data + startPos, size - startPos, identifier.deviceId);
    startPos += GetObject<int32_t>(data + startPos, size - startPos, identifier.vibratorId);
    GetObject<int32_t>(data + startPos, size - startPos, duration);

    int32_t ret = OHOS::Sensors::StartVibratorOnceEnhanced(identifier, duration);
    if (((duration <= MIN_VIBRATOR_TIME || duration > MAX_VIBRATOR_TIME) && ret == 0) ||
        ((duration > MIN_VIBRATOR_TIME && duration <= MAX_VIBRATOR_TIME) && ret != 0)) {
        return false;
    }
    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::StartVibratorOnceEnhancedFuzzTest(data, size);
    return 0;
}