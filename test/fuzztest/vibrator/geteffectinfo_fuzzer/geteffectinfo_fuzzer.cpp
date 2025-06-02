/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "geteffectinfo_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "securec.h"

#include "vibrator_agent.h"

namespace OHOS {
namespace {
constexpr size_t U32_AT_SIZE = 8;
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

bool GetEffectInfoFuzzTest(const uint8_t *data, size_t size)
{
    VibratorIdentifier identifier;
    if (size < U32_AT_SIZE) {
        identifier.deviceId = -1;
        identifier.vibratorId = -1;
    } else {
        size_t startPos = 0;
        startPos += GetObject<int32_t>(data + startPos, size - startPos, identifier.deviceId);
        GetObject<int32_t>(data + startPos, size - startPos, identifier.vibratorId);
    }
    EffectInfo effectInfo;
    std::string effectType = "haptic.clock.timer";
    int32_t ret = OHOS::Sensors::GetEffectInfo(identifier, effectType, effectInfo);
    if (ret == 0) {
        return false;
    }
    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::GetEffectInfoFuzzTest(data, size);
    return 0;
}