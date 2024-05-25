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

#ifndef VIBRATOR_DECODER_MANAGER_H
#define VIBRATOR_DECODER_MANAGER_H

#include "i_vibrator_decoder.h"
#include "json_parser.h"
#include "raw_file_descriptor.h"
#include "vibrator_infos.h"

namespace OHOS {
namespace Sensors {
enum DecoderType {
    DECODER_TYPE_OH_JSON = 0,
    DECODER_TYPE_HE = 1,
    DECODER_TYPE_BUTT,
};

class VibratorDecoderCreator {
public:
    VibratorDecoderCreator() = default;
    virtual ~VibratorDecoderCreator() = default;
    IVibratorDecoder *CreateDecoder(const RawFileDescriptor &rawFd);

private:
    bool CheckHeMetadata(const JsonParser &parser);
    DecoderType GetDecoderType(const RawFileDescriptor &rawFd);
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // VIBRATOR_DECODER_MANAGER_H