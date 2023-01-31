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

#ifndef DEFAULT_VIBRATOR_DECODER_FACTORY_H
#define DEFAULT_VIBRATOR_DECODER_FACTORY_H

#include <cstdint>

#include "vibrator_decoder_factory.h"

namespace OHOS {
namespace Sensors {
class DefaultVibratorDecoderFactory : public VibratorDecoderFactory {
public:
    DefaultVibratorDecoderFactory() = default;
    ~DefaultVibratorDecoderFactory() = default;
    VibratorDecoder *CreateDecoder() override;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // DEFAULT_VIBRATOR_DECODER_FACTORY_H