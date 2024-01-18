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
#include "vibrator_decoder_creator.h"

#include <fcntl.h>
#include <unistd.h>

#include "default_vibrator_decoder_factory.h"
#include "file_utils.h"
#include "he_vibrator_decoder_factory.h"
#include "sensors_errors.h"

#undef LOG_TAG
#define LOG_TAG "VibratorDecoderCreator"

namespace OHOS {
namespace Sensors {

IVibratorDecoder *VibratorDecoderCreator::CreateDecoder(const RawFileDescriptor &fd)
{
    CALL_LOG_ENTER;
    DecoderType type = GetDecoderType(fd);
    if (type == DECODER_TYPE_HE) {
        MISC_HILOGD("Get he decoder");
        HEVibratorDecoderFactory factory;
        return factory.CreateDecoder();
    } else if (type == DECODER_TYPE_OH_JSON) {
        MISC_HILOGD("Get oh_json decoder");
        DefaultVibratorDecoderFactory factory;
        return factory.CreateDecoder();
    }
    MISC_HILOGE("Invalid decoder type");
    return nullptr;
}

DecoderType VibratorDecoderCreator::GetDecoderType(const RawFileDescriptor &rawFd)
{
    std::string extName = "";
    int32_t ret = GetFileExtName(rawFd.fd, extName);
    if (ret != SUCCESS) {
        MISC_HILOGE("GetFileExtName failed");
        return DECODER_TYPE_BUTT;
    }
    if (extName == "he") {
        return DECODER_TYPE_HE;
    } else if (extName == "json") {
        return DECODER_TYPE_OH_JSON;
    } else {
        MISC_HILOGE("Invalid decoder extend name");
        return DECODER_TYPE_BUTT;
    }
}

extern "C" IVibratorDecoder *Create(const RawFileDescriptor &rawFd)
{
    VibratorDecoderCreator creator;
    return creator.CreateDecoder(rawFd);
}

extern "C" void Destroy(IVibratorDecoder *decoder)
{
    if (decoder != nullptr) {
        delete decoder;
        decoder = nullptr;
    }
}
}  // namespace Sensors
}  // namespace OHOS