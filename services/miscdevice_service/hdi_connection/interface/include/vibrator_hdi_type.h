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

 #ifndef VIBRATOR_HDI_TYPE_H
 #define VIBRATOR_HDI_TYPE_H

 #include <vector>

namespace OHOS {
namespace Sensors {
enum VibratorStopMode {
    VIBRATOR_STOP_MODE_TIME = 0,      /**< Indicates the one-shot vibration with the given duration. */
    VIBRATOR_STOP_MODE_PRESET = 1,    /**< Indicates the periodic vibration with the preset effect. */
    VIBRATOR_STOP_MODE_INVALID        /**< Indicates invalid the effect mode. */
};

enum CompositeEffectType {
    COMPOSITE_EFFECT_TYPE_TIME = 0,         /**< Indicates the given duration time effect. */
    COMPOSITE_EFFECT_TYPE_PRIMITIVE = 1,    /**< Indicates the given primitive effect. */
    COMPOSITE_EFFECT_TYPE_INVALID           /**< Indicates invalid the effect type. */
};

struct TimeEffect {
    int32_t delay;
    int32_t time;
    unsigned short intensity;
    short frequency;
};

struct PrimitiveEffect {
    int32_t delay;
    int32_t effectId;
    short intensity;
};

union CompositeEffect {
    struct TimeEffect timeEffect;
    struct PrimitiveEffect primitiveEffect;
};

struct VibratorCompositeEffect {
    CompositeEffectType type;
    std::vector<CompositeEffect> compositeEffects;
};

struct VibratorEffectInfo {
    int32_t duration;
    bool isSupportEffect;
};
} // namespace Sensors
} // namespace OHOS
 #endif // VIBRATOR_HDI_TYPE_H