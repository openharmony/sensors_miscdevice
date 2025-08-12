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

#include "subscribevibrator_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "securec.h"
#include "vibrator_agent.h"

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

void CallbackTest(VibratorStatusEvent *statusEvent)
{
    return;
}

bool SubscribeVibratorFuzzTest(const uint8_t *data, size_t size)
{
    UserData userData;
    GetObject<UserData>(data, size, userData);
    VibratorUser user = {
        .callback = CallbackTest,
        .userData = &userData,
    };
    int32_t ret = OHOS::Sensors::SubscribeVibratorPlug(user);
    if (ret == 0) {
        return false;
    }
    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::SubscribeVibratorFuzzTest(data, size);
    return 0;
}