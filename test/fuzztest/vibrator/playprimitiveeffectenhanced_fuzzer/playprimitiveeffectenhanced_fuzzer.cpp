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

#include "playprimitiveeffectenhanced_fuzzer.h"

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "securec.h"
#include "token_setproc.h"

#include "vibrator_agent.h"

namespace OHOS {
namespace Sensors {
using namespace Security::AccessToken;
using Security::AccessToken::AccessTokenID;

namespace {
constexpr size_t DATA_MIN_SIZE = 28;
constexpr char END_CHAR = '\0';
constexpr size_t LEN = 20;
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

void SetUpTestCase()
{
    const char **perms = new (std::nothrow) const char *[1];
    if (perms == nullptr) {
        return;
    }
    perms[0] = "ohos.permission.VIBRATE";
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "PlayPrimitiveEffectEnhancedFuzzTest",
        .aplStr = "system_core",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;
}

void PlayPrimitiveEffectEnhancedFuzzTest(const uint8_t *data, size_t size)
{
    SetUpTestCase();
    if (data == nullptr || size < DATA_MIN_SIZE) {
        return;
    }
    VibratorIdentifier identifier;
    size_t startPos = 0;
    char effectId[LEN + 1];
    effectId[LEN] = END_CHAR;
    startPos += GetObject<int32_t>(data + startPos, size - startPos, identifier.deviceId);
    startPos += GetObject<int32_t>(data + startPos, size - startPos, identifier.vibratorId);
    for (size_t i = 0; i < LEN; ++i) {
        startPos += GetObject<char>(data + startPos, size - startPos, effectId[i]);
    }
    int32_t intensity { 0 };
    OHOS::Sensors::PlayPrimitiveEffectEnhanced(identifier, effectId, intensity);
}
} // namespace Sensors
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Sensors::PlayPrimitiveEffectEnhancedFuzzTest(data, size);
    return 0;
}