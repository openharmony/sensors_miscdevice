/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef RAW_FILE_DESCRIPTOR
#define RAW_FILE_DESCRIPTOR

#include <cstdint>

namespace OHOS {
namespace Sensors {
struct RawFileDescriptor {
    RawFileDescriptor() = default;
    RawFileDescriptor(int32_t fd, int64_t offset, int64_t length)
        : fd_(fd), offset_(offset), length_(length) {}
    int32_t fd_ = 0;
    int64_t offset_ = 0;
    int64_t length_ = -1;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // RAW_FILE_DESCRIPTOR
