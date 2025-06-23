/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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
#include "vibrator_infos.h"

#include "sensors_errors.h"

#undef LOG_TAG
#define LOG_TAG "MiscdeviceVibratorInfos"

namespace OHOS {
namespace Sensors {
void VibratePattern::Dump() const
{
    int32_t size = static_cast<int32_t>(events.size());
    MISC_HILOGD("Pattern startTime:%{public}d, eventSize:%{public}d", startTime, size);
    for (int32_t i = 0; i < size; ++i) {
        std::string tag = (events[i].tag == EVENT_TAG_CONTINUOUS) ? "continuous" : "transient";
        int32_t pointSize = static_cast<int32_t>(events[i].points.size());
        MISC_HILOGD("Event tag:%{public}s, time:%{public}d, duration:%{public}d,"
            "intensity:%{public}d, frequency:%{public}d, index:%{public}d, curve pointSize:%{public}d",
            tag.c_str(), events[i].time, events[i].duration, events[i].intensity,
            events[i].frequency, events[i].index, pointSize);
        for (int32_t j = 0; j < pointSize; ++j) {
            MISC_HILOGD("Curve point time:%{public}d, intensity:%{public}d, frequency:%{public}d",
                events[i].points[j].time, events[i].points[j].intensity, events[i].points[j].frequency);
        }
    }
}

void VibratePackage::Dump() const
{
    int32_t size = static_cast<int32_t>(patterns.size());
    MISC_HILOGD("Vibrate package pattern size:%{public}d", size);
    for (int32_t i = 0; i < size; ++i) {
        patterns[i].Dump();
    }
}

void VibratorCapacity::Dump() const
{
    std::string isSupportHdHapticStr = isSupportHdHaptic ? "true" : "false";
    std::string isSupportPresetMappingStr = isSupportPresetMapping ? "true" : "false";
    std::string isSupportTimeDelayStr = isSupportTimeDelay ? "true" : "false";
    MISC_HILOGD("SupportHdHaptic:%{public}s, SupportPresetMapping:%{public}s, "
        "SupportTimeDelayStr:%{public}s", isSupportHdHapticStr.c_str(),
        isSupportPresetMappingStr.c_str(), isSupportTimeDelayStr.c_str());
}

int32_t VibratorCapacity::GetVibrateMode()
{
    if (isSupportHdHaptic) {
        return VIBRATE_MODE_HD;
    }
    if (isSupportPresetMapping) {
        return VIBRATE_MODE_MAPPING;
    }
    if (isSupportTimeDelay) {
        return VIBRATE_MODE_TIMES;
    }
    return -1;
}

bool VibratorCapacity::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteBool(isSupportHdHaptic)) {
        MISC_HILOGE("Write isSupportHdHaptic failed");
        return false;
    }
    if (!parcel.WriteBool(isSupportPresetMapping)) {
        MISC_HILOGE("Write isSupportPresetMapping failed");
        return false;
    }
    if (!parcel.WriteBool(isSupportTimeDelay)) {
        MISC_HILOGE("Write isSupportTimeDelay failed");
        return false;
    }
    return true;
}

VibratorCapacity* VibratorCapacity::Unmarshalling(Parcel &data)
{
    auto capacity = new (std::nothrow) VibratorCapacity();
    if (capacity == nullptr) {
        MISC_HILOGE("Read init capacity failed");
        return nullptr;
    }
    if (!(data.ReadBool(capacity->isSupportHdHaptic))) {
        MISC_HILOGE("Read isSupportHdHaptic failed");
        delete capacity;
        capacity = nullptr;
        return capacity;
    }
    if (!(data.ReadBool(capacity->isSupportPresetMapping))) {
        MISC_HILOGE("Read isSupportPresetMapping failed");
        delete capacity;
        capacity = nullptr;
        return capacity;
    }
    if (!(data.ReadBool(capacity->isSupportTimeDelay))) {
        MISC_HILOGE("Read isSupportTimeDelay failed");
        delete capacity;
        capacity = nullptr;
        return capacity;
    }
    return capacity;
}

bool VibratePattern::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(startTime)) {
        MISC_HILOGE("Write pattern's startTime failed");
        return false;
    }
    if (!parcel.WriteInt32(patternDuration)) {
        MISC_HILOGE("Write patternDuration failed");
        return false;
    }
    if (!parcel.WriteInt32(static_cast<int32_t>(events.size()))) {
        MISC_HILOGE("Write events's size failed");
        return false;
    }
    for (size_t i = 0; i < events.size(); ++i) {
        if (!parcel.WriteInt32(static_cast<int32_t>(events[i].tag))) {
            MISC_HILOGE("Write tag failed");
            return false;
        }
        if (!parcel.WriteInt32(events[i].time)) {
            MISC_HILOGE("Write events's time failed");
            return false;
        }
        if (!parcel.WriteInt32(events[i].duration)) {
            MISC_HILOGE("Write duration failed");
            return false;
        }
        if (!parcel.WriteInt32(events[i].intensity)) {
            MISC_HILOGE("Write intensity failed");
            return false;
        }
        if (!parcel.WriteInt32(events[i].frequency)) {
            MISC_HILOGE("Write frequency failed");
            return false;
        }
        if (!parcel.WriteInt32(events[i].index)) {
            MISC_HILOGE("Write index failed");
            return false;
        }
        if (!parcel.WriteInt32(static_cast<int32_t>(events[i].points.size()))) {
            MISC_HILOGE("Write points's size failed");
            return false;
        }
        for (size_t j = 0; j < events[i].points.size(); ++j) {
            if (!parcel.WriteInt32(events[i].points[j].time)) {
                MISC_HILOGE("Write points's time failed");
                return false;
            }
            if (!parcel.WriteInt32(events[i].points[j].intensity)) {
                MISC_HILOGE("Write points's intensity failed");
                return false;
            }
            if (!parcel.WriteInt32(events[i].points[j].frequency)) {
                MISC_HILOGE("Write points's frequency failed");
                return false;
            }
        }
    }
    return true;
}

VibratePattern* VibratePattern::Unmarshalling(Parcel &data)
{
    auto pattern = new (std::nothrow) VibratePattern();
    if (pattern == nullptr || !(data.ReadInt32(pattern->startTime)) || !(data.ReadInt32(pattern->patternDuration))) {
        MISC_HILOGE("Read pattern basic info failed");
        if (pattern != nullptr) {
            delete pattern;
            pattern = nullptr;
        }
        return pattern;
    }
    int32_t eventSize{ 0 };
    if (!(data.ReadInt32(eventSize)) || eventSize > MAX_EVENT_SIZE) {
        MISC_HILOGE("Read eventSize failed or eventSize exceed the maximum");
        delete pattern;
        pattern = nullptr;
        return pattern;
    }
    for (int32_t i = 0; i < eventSize; ++i) {
        VibrateEvent event;
        int32_t tag{ -1 };
        if (!data.ReadInt32(tag)) {
            MISC_HILOGE("Read type failed");
            delete pattern;
            pattern = nullptr;
            return pattern;
        }
        event.tag = static_cast<VibrateTag>(tag);
        if (!data.ReadInt32(event.time) || !data.ReadInt32(event.duration) || !data.ReadInt32(event.intensity) ||
            !data.ReadInt32(event.frequency) || !data.ReadInt32(event.index)) {
            MISC_HILOGE("Read events info failed");
            delete pattern;
            pattern = nullptr;
            return pattern;
        }
        int32_t pointSize{ 0 };
        if (!data.ReadInt32(pointSize) || pointSize > MAX_POINT_SIZE) {
            MISC_HILOGE("Read pointSize failed or pointSize exceed the maximum");
            delete pattern;
            pattern = nullptr;
            return pattern;
        }
        pattern->events.emplace_back(event);
        for (int32_t j = 0; j < pointSize; ++j) {
            VibrateCurvePoint point;
            if (!data.ReadInt32(point.time) || !data.ReadInt32(point.intensity) || !data.ReadInt32(point.frequency)) {
                MISC_HILOGE("Read points info time failed");
                delete pattern;
                pattern = nullptr;
                return pattern;
            }
            pattern->events[i].points.emplace_back(point);
        }
    }
    return pattern;
}

bool VibratePackageIPC::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(packageDuration)) {
        MISC_HILOGE("Write packageDuration failed");
        return false;
    }
    if (!parcel.WriteInt32(patternNum)) {
        MISC_HILOGE("Write pattern's patternNum failed");
        return false;
    }
    if (patternNum != static_cast<int32_t>(patterns.size())) {
        MISC_HILOGE("patternNum does not match patterns size");
        return false;
    }
    for (int32_t i = 0; i < patternNum; ++i) {
        if (!parcel.WriteInt32(patterns[i].startTime)) {
            MISC_HILOGE("Write pattern's startTime failed");
            return false;
        }
        if (!parcel.WriteInt32(patterns[i].patternDuration)) {
            MISC_HILOGE("Write patternDuration failed");
            return false;
        }
        if (!parcel.WriteInt32(static_cast<int32_t>(patterns[i].events.size()))) {
            MISC_HILOGE("Write events's size failed");
            return false;
        }
        for (size_t j = 0; j < patterns[i].events.size(); ++j) {
            if (!parcel.WriteInt32(static_cast<int32_t>(patterns[i].events[j].tag))) {
                MISC_HILOGE("Write tag failed");
                return false;
            }
            if (!parcel.WriteInt32(patterns[i].events[j].time)) {
                MISC_HILOGE("Write events's time failed");
                return false;
            }
            if (!parcel.WriteInt32(patterns[i].events[j].duration)) {
                MISC_HILOGE("Write duration failed");
                return false;
            }
            if (!parcel.WriteInt32(patterns[i].events[j].intensity)) {
                MISC_HILOGE("Write intensity failed");
                return false;
            }
            if (!parcel.WriteInt32(patterns[i].events[j].frequency)) {
                MISC_HILOGE("Write frequency failed");
                return false;
            }
            if (!parcel.WriteInt32(patterns[i].events[j].index)) {
                MISC_HILOGE("Write index failed");
                return false;
            }
            if (!parcel.WriteInt32(static_cast<int32_t>(patterns[i].events[j].points.size()))) {
                MISC_HILOGE("Write points's size failed");
                return false;
            }
            for (size_t k = 0; k < patterns[i].events[j].points.size(); ++k) {
                if (!parcel.WriteInt32(patterns[i].events[j].points[k].time)) {
                    MISC_HILOGE("Write points's time failed");
                    return false;
                }
                if (!parcel.WriteInt32(patterns[i].events[j].points[k].intensity)) {
                    MISC_HILOGE("Write points's intensity failed");
                    return false;
                }
                if (!parcel.WriteInt32(patterns[i].events[j].points[k].frequency)) {
                    MISC_HILOGE("Write points's frequency failed");
                    return false;
                }
            }
        }
    }
    return true;
}

VibratePackageIPC* VibratePackageIPC::Unmarshalling(Parcel &data)
{
    auto package = new (std::nothrow) VibratePackageIPC();
    if (package == nullptr || !(data.ReadInt32(package->packageDuration))) {
        MISC_HILOGE("Read pattern basic info failed");
        if (package != nullptr) {
            delete package;
            package = nullptr;
        }
        return package;
    }
    int32_t patternNum{ 0 };
    if (!(data.ReadInt32(patternNum)) || patternNum > MAX_EVENT_SIZE) {
        MISC_HILOGE("Read patternNum failed or patternNum exceed the maximum");
        delete package;
        package = nullptr;
        return package;
    }
    package->patternNum = patternNum;
    for (int32_t i = 0; i < patternNum; ++i) {
        VibratePattern pattern;
        if (!(data.ReadInt32(pattern.startTime)) || !(data.ReadInt32(pattern.patternDuration))) {
            MISC_HILOGE("Read pattern basic info failed");
            delete package;
            package = nullptr;
            return package;
        }
        int32_t eventSize{ 0 };
        if (!(data.ReadInt32(eventSize)) || eventSize > MAX_EVENT_SIZE || eventSize < 0) {
            MISC_HILOGE("Read eventSize failed or eventSize exceed the maximum");
            delete package;
            package = nullptr;
            return package;
        }
        for (int32_t j = 0; j < eventSize; ++j) {
            VibrateEvent event;
            int32_t tag{ -1 };
            if (!data.ReadInt32(tag)) {
                MISC_HILOGE("Read type failed");
                delete package;
                package = nullptr;
                return package;
            }
            event.tag = static_cast<VibrateTag>(tag);
            if (!data.ReadInt32(event.time) || !data.ReadInt32(event.duration) || !data.ReadInt32(event.intensity) ||
                !data.ReadInt32(event.frequency) || !data.ReadInt32(event.index)) {
                MISC_HILOGE("Read events info failed");
                delete package;
                package = nullptr;
                return package;
            }
            int32_t pointSize{ 0 };
            if (!data.ReadInt32(pointSize) || pointSize > MAX_POINT_SIZE || pointSize < 0) {
                MISC_HILOGE("Read pointSize failed or pointSize exceed the maximum");
                delete package;
                package = nullptr;
                return package;
            }
            for (int32_t k = 0; k < pointSize; ++k) {
                VibrateCurvePoint point;
                if (!data.ReadInt32(point.time) || !data.ReadInt32(point.intensity) ||
                    !data.ReadInt32(point.frequency)) {
                    MISC_HILOGE("Read points info time failed");
                    delete package;
                    package = nullptr;
                    return package;
                }
                event.points.emplace_back(point);
            }
            pattern.events.emplace_back(event);
        }
        package->patterns.emplace_back(pattern);
    }
    return package;
}

void VibratePackageIPC::Dump() const
{
    int32_t size = static_cast<int32_t>(patterns.size());
    MISC_HILOGD("Vibrate package pattern size:%{public}d", size);
    for (int32_t i = 0; i < size; ++i) {
        patterns[i].Dump();
    }
}

void VibrateParameter::Dump() const
{
    MISC_HILOGI("intensity:%{public}d, frequency:%{public}d", intensity, frequency);
}

bool VibrateParameter::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(intensity)) {
        MISC_HILOGE("Write parameter's intensity failed");
        return false;
    }
    if (!parcel.WriteInt32(frequency)) {
        MISC_HILOGE("Write parameter's frequency failed");
        return false;
    }
    return true;
}

VibrateParameter* VibrateParameter::Unmarshalling(Parcel &data)
{
    auto parameter = new (std::nothrow) VibrateParameter();
    if (parameter == nullptr) {
        MISC_HILOGE("Read init parameter failed");
        return nullptr;
    }
    if (!(data.ReadInt32(parameter->intensity)) && !(data.ReadInt32(parameter->frequency))) {
        MISC_HILOGE("Read parameter's intensity failed");
        delete parameter;
        parameter = nullptr;
    }
    return parameter;
}

bool VibratorInfoIPC::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(deviceId)) {
        MISC_HILOGE("Write deviceId failed");
        return false;
    }
    if (!parcel.WriteInt32(vibratorId)) {
        MISC_HILOGE("Write vibratorId failed");
        return false;
    }
    if (!parcel.WriteString(deviceName)) {
        MISC_HILOGE("Write deviceName failed");
        return false;
    }
    if (!parcel.WriteBool(isSupportHdHaptic)) {
        MISC_HILOGE("Write isSupportHdHaptic failed");
        return false;
    }
    if (!parcel.WriteBool(isLocalVibrator)) {
        MISC_HILOGE("Write isLocalVibrator failed");
        return false;
    }
    return true;
}

VibratorInfoIPC* VibratorInfoIPC::Unmarshalling(Parcel &data)
{
    auto vibratorInfoIPC = new (std::nothrow) VibratorInfoIPC();
    if (vibratorInfoIPC == nullptr) {
        MISC_HILOGE("Read init vibratorInfoIPC failed");
        return nullptr;
    }
    if (!data.ReadInt32(vibratorInfoIPC->deviceId)) {
        MISC_HILOGE("Read deviceId failed");
        delete vibratorInfoIPC;
        vibratorInfoIPC = nullptr;
        return vibratorInfoIPC;
    }
    if (!data.ReadInt32(vibratorInfoIPC->vibratorId)) {
        MISC_HILOGE("Read vibratorId failed");
        delete vibratorInfoIPC;
        vibratorInfoIPC = nullptr;
        return vibratorInfoIPC;
    }
    if (!data.ReadString(vibratorInfoIPC->deviceName)) {
        MISC_HILOGE("Read deviceName failed");
        delete vibratorInfoIPC;
        vibratorInfoIPC = nullptr;
        return vibratorInfoIPC;
    }
    if (!data.ReadBool(vibratorInfoIPC->isSupportHdHaptic)) {
        MISC_HILOGE("Read isSupportHdHaptic failed");
        delete vibratorInfoIPC;
        vibratorInfoIPC = nullptr;
        return vibratorInfoIPC;
    }
    if (!data.ReadBool(vibratorInfoIPC->isLocalVibrator)) {
        MISC_HILOGE("Read isLocalVibrator failed");
        delete vibratorInfoIPC;
        vibratorInfoIPC = nullptr;
        return vibratorInfoIPC;
    }
    return vibratorInfoIPC;
}

void VibratorInfoIPC::Dump() const
{
    std::string retStr = "";
    retStr = "deviceId: "+ std::to_string(deviceId) + " ";
    retStr += ("vibratorId: " + std::to_string(vibratorId) + " ");
    retStr += ("deviceName: " + deviceName + " ");
    retStr += ("isSupportHdHaptic: " + (isSupportHdHaptic? std::string("true"):std::string("false")) + " ");
    MISC_HILOGI("VibratorInfoIPC: [%{public}s]", retStr.c_str());
}

void VibratorIdentifierIPC::Dump() const
{
    MISC_HILOGI("deviceId:%{public}d, vibratorId:%{public}d", deviceId, vibratorId);
}

bool VibratorIdentifierIPC::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(deviceId)) {
        MISC_HILOGE("Write parameter's deviceId failed");
        return false;
    }
    if (!parcel.WriteInt32(vibratorId)) {
        MISC_HILOGE("Write parameter's vibratorId failed");
        return false;
    }
    return true;
}

VibratorIdentifierIPC* VibratorIdentifierIPC::Unmarshalling(Parcel &data)
{
    auto identifier = new (std::nothrow) VibratorIdentifierIPC();
    if (identifier == nullptr) {
        MISC_HILOGE("Read init parameter failed");
        return nullptr;
    }
    if (!(data.ReadInt32(identifier->deviceId))) {
        MISC_HILOGE("Read parameter's deviceId or vibratorId failed");
        delete identifier;
        identifier = nullptr;
        return identifier;
    }
    if (!(data.ReadInt32(identifier->vibratorId))) {
        MISC_HILOGE("Read parameter's deviceId or vibratorId failed");
        delete identifier;
        identifier = nullptr;
        return identifier;
    }
    return identifier;
}

bool EffectInfoIPC::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(duration)) {
        MISC_HILOGE("Write duration failed");
        return false;
    }
    if (!parcel.WriteBool(isSupportEffect)) {
        MISC_HILOGE("Write isSupportEffect failed");
        return false;
    }
    return true;
}

EffectInfoIPC* EffectInfoIPC::Unmarshalling(Parcel &data)
{
    auto effectInfoIPC = new (std::nothrow) EffectInfoIPC();
    if (effectInfoIPC == nullptr) {
        MISC_HILOGE("Read init EffectInfoIPC failed");
        return nullptr;
    }
    if (!data.ReadInt32(effectInfoIPC->duration)) {
        MISC_HILOGE("Read duration failed");
        delete effectInfoIPC;
        effectInfoIPC = nullptr;
        return effectInfoIPC;
    }
    if (!data.ReadBool(effectInfoIPC->isSupportEffect)) {
        MISC_HILOGE("Read isSupportEffect failed");
        delete effectInfoIPC;
        effectInfoIPC = nullptr;
        return effectInfoIPC;
    }
    return effectInfoIPC;
}

void EffectInfoIPC::Dump() const
{
    std::string retStr = "";
    retStr = "duration: "+ std::to_string(duration) + " ";
    retStr += ("isSupportEffect: " + (isSupportEffect? std::string("true"):std::string("false")));
    MISC_HILOGI("EffectInfoIPC: [%{public}s]", retStr.c_str());
}

bool CustomHapticInfoIPC::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(usage)) {
        MISC_HILOGE("Write usage failed");
        return false;
    }
    if (!parcel.WriteBool(systemUsage)) {
        MISC_HILOGE("Write systemUsage failed");
        return false;
    }
    if (!parcel.WriteUint32(parameter.sessionId)) {
        MISC_HILOGE("Write sessionId failed");
        return false;
    }
    if (!parcel.WriteInt32(parameter.intensity)) {
        MISC_HILOGE("Write intensity failed");
        return false;
    }
    if (!parcel.WriteInt32(parameter.frequency)) {
        MISC_HILOGE("Write frequency failed");
        return false;
    }
    if (!parcel.WriteInt32(parameter.reserved)) {
        MISC_HILOGE("Write reserved failed");
        return false;
    }
    return true;
}

CustomHapticInfoIPC* CustomHapticInfoIPC::Unmarshalling(Parcel &data)
{
    auto customHapticInfoIPC = new (std::nothrow) CustomHapticInfoIPC();
    if (customHapticInfoIPC == nullptr) {
        MISC_HILOGE("Read init EffectInfoIPC failed");
        return nullptr;
    }
    if (!data.ReadInt32(customHapticInfoIPC->usage)) {
        MISC_HILOGE("Read usage failed");
        delete customHapticInfoIPC;
        customHapticInfoIPC = nullptr;
        return customHapticInfoIPC;
    }
    if (!data.ReadBool(customHapticInfoIPC->systemUsage)) {
        MISC_HILOGE("Read systemUsage failed");
        delete customHapticInfoIPC;
        customHapticInfoIPC = nullptr;
        return customHapticInfoIPC;
    }
    if (!data.ReadUint32(customHapticInfoIPC->parameter.sessionId)) {
        MISC_HILOGE("Read sessionId failed");
        customHapticInfoIPC = nullptr;
        return customHapticInfoIPC;
    }
    if (!data.ReadInt32(customHapticInfoIPC->parameter.intensity)) {
        MISC_HILOGE("Read intensity failed");
        delete customHapticInfoIPC;
        customHapticInfoIPC = nullptr;
        return customHapticInfoIPC;
    }
    if (!data.ReadInt32(customHapticInfoIPC->parameter.frequency)) {
        MISC_HILOGE("Read frequency failed");
        delete customHapticInfoIPC;
        customHapticInfoIPC = nullptr;
        return customHapticInfoIPC;
    }
    if (!data.ReadInt32(customHapticInfoIPC->parameter.reserved)) {
        MISC_HILOGE("Read reserved failed");
        delete customHapticInfoIPC;
        customHapticInfoIPC = nullptr;
        return customHapticInfoIPC;
    }
    return customHapticInfoIPC;
}

void CustomHapticInfoIPC::Dump() const
{
    std::string retStr = "";
    retStr = "usage: "+ std::to_string(usage) + " ";
    retStr += "systemUsage: " + std::to_string(systemUsage)  + " ";
    retStr += "parameter.intensity: " + std::to_string(parameter.intensity)  + " ";
    retStr += "parameter.frequency: " + std::to_string(parameter.frequency)  + " ";
    retStr += "parameter.reserved: " + std::to_string(parameter.reserved)  + " ";
    MISC_HILOGI("CustomHapticInfoIPC: [%{public}s]", retStr.c_str());
}

bool PrimitiveEffectIPC::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(intensity)) {
        MISC_HILOGE("Write intensity failed");
        return false;
    }
    if (!parcel.WriteInt32(usage)) {
        MISC_HILOGE("Write usage failed");
        return false;
    }
    if (!parcel.WriteBool(systemUsage)) {
        MISC_HILOGE("Write systemUsage failed");
        return false;
    }
    if (!parcel.WriteInt32(count)) {
        MISC_HILOGE("Write count failed");
        return false;
    }
    return true;
}

PrimitiveEffectIPC* PrimitiveEffectIPC::Unmarshalling(Parcel &data)
{
    auto primitiveEffectIPC = new (std::nothrow) PrimitiveEffectIPC();
    if (primitiveEffectIPC == nullptr) {
        MISC_HILOGE("Read init PrimitiveEffectIPC failed");
        return nullptr;
    }
    if (!data.ReadInt32(primitiveEffectIPC->intensity)) {
        MISC_HILOGE("Read intensity failed");
        delete primitiveEffectIPC;
        primitiveEffectIPC = nullptr;
        return primitiveEffectIPC;
    }
    if (!data.ReadInt32(primitiveEffectIPC->usage)) {
        MISC_HILOGE("Read usage failed");
        delete primitiveEffectIPC;
        primitiveEffectIPC = nullptr;
        return primitiveEffectIPC;
    }

    if (!data.ReadBool(primitiveEffectIPC->systemUsage)) {
        MISC_HILOGE("Read systemUsage failed");
        delete primitiveEffectIPC;
        primitiveEffectIPC = nullptr;
        return primitiveEffectIPC;
    }

    if (!data.ReadInt32(primitiveEffectIPC->count)) {
        MISC_HILOGE("Read count failed");
        delete primitiveEffectIPC;
        primitiveEffectIPC = nullptr;
        return primitiveEffectIPC;
    }
    return primitiveEffectIPC;
}

void PrimitiveEffectIPC::Dump() const
{
    std::string retStr = "";
    retStr = "intensity: "+ std::to_string(intensity) + " ";
    retStr += "usage: " + std::to_string(usage)  + " ";
    retStr += "systemUsage: " + std::to_string(systemUsage)  + " ";
    retStr += "count: " + std::to_string(count)  + " ";
    MISC_HILOGI("PrimitiveEffectIPC: [%{public}s]", retStr.c_str());
}

} // namespace Sensors
} // namespace OHOS
