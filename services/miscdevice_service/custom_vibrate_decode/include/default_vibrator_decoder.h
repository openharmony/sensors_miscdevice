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

#ifndef DEFAULT_VIBRATOR_DECODER_H
#define DEFAULT_VIBRATOR_DECODER_H

#include <cstdint>
#include <vector>

#include "json_parser.h"
#include "vibrator_decoder.h"

namespace OHOS {
namespace Sensors {
class DefaultVibratorDecoder : public VibratorDecoder {
public:
    DefaultVibratorDecoder() = default;
    ~DefaultVibratorDecoder() = default;
    std::set<VibrateEvent> DecodeEffect(const RawFileDescriptor &rawFd) override;

private:
    int32_t CheckMetadata(const JsonParser &parser);
    int32_t ParseChannel(const JsonParser &parser, std::set<VibrateEvent> &vibrateSet);
    int32_t ParseChannelParameters(const JsonParser &parser, cJSON *channelParameters);
    int32_t ParsePattern(const JsonParser &parser, cJSON *pattern, std::set<VibrateEvent> &vibrateSet);
    int32_t ParseEvent(const JsonParser &parser, cJSON *event, std::set<VibrateEvent> &vibrateSet);
    bool CheckParameters(const VibrateEvent &vibrateEvent);
    int32_t channelNumber_ = 0;
    double version_ = 0.0;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // VIBRATOR_DECODER_H