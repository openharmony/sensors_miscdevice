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

#include "getvibratoridlist_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "vibrator_agent.h"

namespace OHOS {
bool GetVibratorIdListFuzzTest(const uint8_t *data, size_t size)
{
    VibratorIdentifier identifier;
 
    if (size < sizeof(int32_t) * 2) {
        identifier.deviceId = -1;  
        identifier.vibratorId = -1;  
    } else {
        std::memcpy(&identifier.deviceId, data, sizeof(int32_t));
        std::memcpy(&identifier.vibratorId, data + sizeof(int32_t), sizeof(int32_t));
    }
    std::vector<VibratorInfos> vibratorInfo;
    int32_t ret = OHOS::Sensors::GetVibratorIdList(identifier, vibratorInfo);
    if (ret == 0) {
        return false;
    }
    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::GetVibratorIdListFuzzTest(data, size);
    return 0;
}