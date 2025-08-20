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

#include "handlevibratordata_fuzzer.h"

#include "securec.h"
#include "vibrator_service_client.h"

namespace OHOS {
namespace Sensors {
namespace {
constexpr size_t DATA_MIN_SIZE = 8;
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

void HandleVibratorDataFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < DATA_MIN_SIZE) {
        return;
    }
    size_t startPos = 0;
    VibratorStatusEvent statusEvent;
    startPos += GetObject<int32_t>(data + startPos, size - startPos, statusEvent.deviceId);
    GetObject<int32_t>(data + startPos, size - startPos, statusEvent.vibratorCnt);
    VibratorServiceClient::GetInstance().HandleVibratorData(statusEvent);
}
} // namespace Sensors
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Sensors::HandleVibratorDataFuzzTest(data, size);
    return 0;
}