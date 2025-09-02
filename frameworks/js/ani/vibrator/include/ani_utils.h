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

#ifndef ANI_UTILS_H
#define ANI_UTILS_H

#include <ani.h>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
class AniStringUtils {
public:
    static std::string ToStd(ani_env *env, ani_string ani_str)
    {
        ani_size strSize;
        env->String_GetUTF8Size(ani_str, &strSize);

        std::vector<char> buffer(strSize + 1); // +1 for null terminator
        char* utf8_buffer = buffer.data();

        //String_GetUTF8 Supportted by https://gitee.com/openharmony/arkcompiler_runtime_core/pulls/3416
        ani_size bytes_written = 0;
        env->String_GetUTF8(ani_str, utf8_buffer, strSize + 1, &bytes_written);

        utf8_buffer[bytes_written] = '\0';
        std::string content = std::string(utf8_buffer);
        return content;
    }

    static ani_string ToAni(ani_env* env, const std::string& str)
    {
        ani_string aniStr = nullptr;
        if (ANI_OK != env->String_NewUTF8(str.data(), str.size(), &aniStr)) {
            std::cerr << "[ANI] Unsupported ANI_VERSION_1" << std::endl;
            return nullptr;
        }
        return aniStr;
    }
};

class UnionAccessor {
public:
    UnionAccessor(ani_env *env, ani_object &obj) : env_(env), obj_(obj)
    {
    }

    bool IsInstanceOf(const std::string& cls_name)
    {
        ani_class cls;
        env_->FindClass(cls_name.c_str(), &cls);

        ani_boolean ret;
        env_->Object_InstanceOf(obj_, cls, &ret);
        return ret;
    }

    template<typename T>
    bool IsInstanceOfType();

    template<typename T>
    bool TryConvert(T &value);

    template<typename T>
    bool TryConvertArray(std::vector<T> &value);

private:
    ani_env *env_;
    ani_object obj_;
};

template<>
bool UnionAccessor::IsInstanceOfType<bool>()
{
    return IsInstanceOf("Lstd/core/Boolean;");
}

template<>
bool UnionAccessor::IsInstanceOfType<int>()
{
    return IsInstanceOf("Lstd/core/Int;");
}

template<>
bool UnionAccessor::IsInstanceOfType<double>()
{
    return IsInstanceOf("Lstd/core/Double;");
}

template<>
bool UnionAccessor::IsInstanceOfType<std::string>()
{
    return IsInstanceOf("Lstd/core/String;");
}

template<>
bool UnionAccessor::TryConvert<bool>(bool &value)
{
    if (!IsInstanceOfType<bool>()) {
        return false;
    }

    ani_boolean aniValue;
    auto ret = env_->Object_CallMethodByName_Boolean(obj_, "unboxed", nullptr, &aniValue);
    if (ret != ANI_OK) {
        return false;
    }
    value = static_cast<bool>(aniValue);
    return true;
}

template<>
bool UnionAccessor::TryConvert<int>(int &value)
{
    if (!IsInstanceOfType<int>()) {
        return false;
    }

    ani_int aniValue;
    auto ret = env_->Object_CallMethodByName_Int(obj_, "unboxed", nullptr, &aniValue);
    if (ret != ANI_OK) {
        return false;
    }
    value = static_cast<int>(aniValue);
    return true;
}

template<>
bool UnionAccessor::TryConvert<double>(double &value)
{
    if (!IsInstanceOfType<double>()) {
        return false;
    }

    ani_double aniValue;
    auto ret = env_->Object_CallMethodByName_Double(obj_, "unboxed", nullptr, &aniValue);
    if (ret != ANI_OK) {
        return false;
    }
    value = static_cast<double>(aniValue);
    return true;
}

template<>
bool UnionAccessor::TryConvert<std::string>(std::string &value)
{
    if (!IsInstanceOfType<std::string>()) {
        return false;
    }

    value = AniStringUtils::ToStd(env_, static_cast<ani_string>(obj_));
    return true;
}

template<>
bool UnionAccessor::TryConvertArray<bool>(std::vector<bool> &value)
{
    ani_double length;
    if (ANI_OK != env_->Object_GetPropertyByName_Double(obj_, "length", &length)) {
        std::cerr << "Object_GetPropertyByName_Double length failed" << std::endl;
        return false;
    }
    int64_t lengthValue = static_cast<int64_t>(length);
    if (lengthValue < 0 || lengthValue > INT64_MAX) {
        std::cerr << "Invalid length" << std::endl;
        return false;
    }
    for (int64_t i = 0; i < lengthValue; i++) {
        ani_ref ref;
        if (ANI_OK != env_->Object_CallMethodByName_Ref(obj_, "$_get", "I:Lstd/core/Object;", &ref, (ani_int)i)) {
            std::cerr << "Object_GetPropertyByName_Ref failed" << std::endl;
            return false;
        }
        ani_boolean val;
        if (ANI_OK != env_->Object_CallMethodByName_Boolean(static_cast<ani_object>(ref), "unboxed", nullptr, &val)) {
            std::cerr << "Object_CallMethodByName_Double unbox failed" << std::endl;
            return false;
        }
        value.push_back(static_cast<bool>(val));
    }
    return true;
}

template<>
bool UnionAccessor::TryConvertArray<int>(std::vector<int> &value)
{
    ani_double length;
    if (ANI_OK != env_->Object_GetPropertyByName_Double(obj_, "length", &length)) {
        std::cerr << "Object_GetPropertyByName_Double length failed" << std::endl;
        return false;
    }
    int64_t lengthValue = static_cast<int64_t>(length);
    if (lengthValue < 0 || lengthValue > INT64_MAX) {
        std::cerr << "Invalid length" << std::endl;
        return false;
    }
    for (int64_t i = 0; i < lengthValue; i++) {
        ani_ref ref;
        if (ANI_OK != env_->Object_CallMethodByName_Ref(obj_, "$_get", "I:Lstd/core/Object;", &ref, (ani_int)i)) {
            std::cerr << "Object_GetPropertyByName_Ref failed" << std::endl;
            return false;
        }
        ani_int intValue;
        if (ANI_OK != env_->Object_CallMethodByName_Int(static_cast<ani_object>(ref), "unboxed", nullptr, &intValue)) {
            std::cerr << "Object_CallMethodByName_Double unbox failed" << std::endl;
            return false;
        }
        value.push_back(static_cast<int>(intValue));
    }
    return true;
}

template<>
bool UnionAccessor::TryConvertArray<double>(std::vector<double> &value)
{
    ani_double length;
    if (ANI_OK != env_->Object_GetPropertyByName_Double(obj_, "length", &length)) {
        std::cerr << "Object_GetPropertyByName_Double length failed" << std::endl;
        return false;
    }
    int64_t lengthValue = static_cast<int64_t>(length);
    if (lengthValue < 0 || lengthValue > INT64_MAX) {
        std::cerr << "Invalid length" << std::endl;
        return false;
    }
    for (int64_t i = 0; i < lengthValue; i++) {
        ani_ref ref;
        if (ANI_OK != env_->Object_CallMethodByName_Ref(obj_, "$_get", "I:Lstd/core/Object;", &ref, (ani_int)i)) {
            std::cerr << "Object_GetPropertyByName_Ref failed" << std::endl;
            return false;
        }
        ani_double val;
        if (ANI_OK != env_->Object_CallMethodByName_Double(static_cast<ani_object>(ref), "unboxed", nullptr, &val)) {
            std::cerr << "Object_CallMethodByName_Double unbox failed" << std::endl;
            return false;
        }
        value.push_back(static_cast<double>(val));
    }
    return true;
}

template<>
bool UnionAccessor::TryConvertArray<uint8_t>(std::vector<uint8_t> &value)
{
    std::cout << "TryConvertArray std::vector<uint8_t>" << std::endl;
    ani_ref buffer;
    if (ANI_OK != env_->Object_GetFieldByName_Ref(obj_, "buffer", &buffer)) {
        std::cout << "Object_GetFieldByName_Ref failed" << std::endl;
        return false;
    }
    void* data;
    size_t length;
    if (ANI_OK != env_->ArrayBuffer_GetInfo(static_cast<ani_arraybuffer>(buffer), &data, &length)) {
        std::cerr << "ArrayBuffer_GetInfo failed" << std::endl;
        return false;
    }
    std::cout << "Length of buffer is " << length << std::endl;
    for (size_t i = 0; i < length; i++) {
        value.push_back(static_cast<uint8_t*>(data)[i]);
    }
    return true;
}

template<>
bool UnionAccessor::TryConvertArray<std::string>(std::vector<std::string> &value)
{
    ani_double length;
    if (ANI_OK != env_->Object_GetPropertyByName_Double(obj_, "length", &length)) {
        std::cerr << "Object_GetPropertyByName_Double length failed" << std::endl;
        return false;
    }
    int64_t lengthValue = static_cast<int64_t>(length);
    if (lengthValue < 0 || lengthValue > INT64_MAX) {
        std::cerr << "Invalid length" << std::endl;
        return false;
    }
    for (int64_t i = 0; i < lengthValue; i++) {
        ani_ref ref;
        if (ANI_OK != env_->Object_CallMethodByName_Ref(obj_, "$_get", "I:Lstd/core/Object;", &ref, (ani_int)i)) {
            std::cerr << "Object_GetPropertyByName_Double length failed" << std::endl;
            return false;
        }
        value.push_back(AniStringUtils::ToStd(env_, static_cast<ani_string>(ref)));
    }
    return true;
}

class OptionalAccessor {
public:
    OptionalAccessor(ani_env *env, ani_object &obj) : env_(env), obj_(obj)
    {
    }

    bool IsUndefined()
    {
        ani_boolean isUndefined;
        env_->Reference_IsUndefined(obj_, &isUndefined);
        return isUndefined;
    }

    template<typename T>
    std::optional<T> Convert();

private:
    ani_env *env_;
    ani_object obj_;
};

template<>
std::optional<double> OptionalAccessor::Convert<double>()
{
    if (IsUndefined()) {
        return std::nullopt;
    }

    ani_double aniValue;
    auto ret = env_->Object_CallMethodByName_Double(obj_, "doubleValue", nullptr, &aniValue);
    if (ret != ANI_OK) {
        return std::nullopt;
    }
    auto value = static_cast<double>(aniValue);
    return value;
}

template<>
std::optional<std::string> OptionalAccessor::Convert<std::string>()
{
    if (IsUndefined()) {
        return std::nullopt;
    }

    ani_size strSize;
    auto status = env_->String_GetUTF8Size(static_cast<ani_string>(obj_), &strSize);
    if (status != ANI_OK) {
        return std::nullopt;
    }

    std::vector<char> buffer(strSize + 1);
    char* utf8_buffer = buffer.data();

    ani_size bytes_written = 0;
    env_->String_GetUTF8(static_cast<ani_string>(obj_), utf8_buffer, strSize + 1, &bytes_written);

    utf8_buffer[bytes_written] = '\0';
    std::string content = std::string(utf8_buffer);
    return content;
}

template <typename T, typename E>
class expected {
private:
    std::variant<T, E> data_;
    bool has_value_;

public:
    expected(const T &value)
        : data_(value), has_value_(true)
    {
    }

    expected(T &&value)
        : data_(std::move(value)), has_value_(true)
    {
    }

    expected(const E &error)
        : data_(error), has_value_(false)
    {
    }

    expected(E &&error)
        : data_(std::move(error)), has_value_(false)
    {
    }

    bool has_value() const noexcept
    {
        return has_value_;
    }

    explicit operator bool() const noexcept
    {
        return has_value();
    }

    T &value() &
    {
        if (!has_value()) {
            std::terminate();
        }
        return std::get<T>(data_);
    }

    const T &value() const &
    {
        if (!has_value()) {
            std::terminate();
        }
        return std::get<T>(data_);
    }

    T &&value() &&
    {
        if (!has_value()) {
            std::terminate();
        }
        return std::get<T>(std::move(data_));
    }

    E &error() &
    {
        if (has_value()) {
            std::terminate();
        }
        return std::get<E>(data_);
    }

    const E &error() const &
    {
        if (has_value()) {
            std::terminate();
        }
        return std::get<E>(data_);
    }

    E &&error() &&
    {
        if (has_value()) {
            std::terminate();
        }
        return std::get<E>(std::move(data_));
    }

    T &operator*() &
    {
        return value();
    }

    const T &operator*() const &
    {
        return value();
    }

    T &&operator*() &&
    {
        return std::move(*this).value();
    }

    T *operator->()
    {
        return &value();
    }

    const T *operator->() const
    {
        return &value();
    }

    template <typename U>
    T value_or(U &&default_value) const &
    {
        return has_value() ? value() : static_cast<T>(std::forward<U>(default_value));
    }

    template <typename U>
    T value_or(U &&default_value) &&
    {
        return has_value() ? std::move(*this).value() : static_cast<T>(std::forward<U>(default_value));
    }
};

class EnumAccessor {
    public:
        EnumAccessor(ani_env *env, const char* className, ani_int index) : env_(env)
        {
            initStatus_ = ANI_ERROR;
            ani_enum_item item;
            initStatus_ = GetItem(className, index, item);
            if (ANI_OK == initStatus_) {
                item_ = item;
            }
        }

        EnumAccessor(ani_env *env, ani_enum_item item) : env_(env), item_(item)
        {
            initStatus_ = ANI_ERROR;
        }

        template<typename T>
        expected<T, ani_status> To()
        {
            int32_t value{};
            ani_status status = ToInt(value);
            if (ANI_OK != status) {
                return status;
            }
            return static_cast<T>(value);
        }

        ani_status ToInt(int32_t &value)
        {
            if (!item_) {
                return initStatus_;
            }

            ani_status status = env_->EnumItem_GetValue_Int(item_.value(), &value);
            if (ANI_OK != status) {
                return status;
            }
            return ANI_OK;
        }

        expected<int32_t, ani_status> ToInt()
        {
            int32_t value;
            ani_status status = ToInt(value);
            if (ANI_OK != status) {
                return status;
            }
            return value;
        }

        ani_status ToString(std::string &value)
        {
            if (!item_) {
                return initStatus_;
            }

            ani_string strValue;
            ani_status status = env_->EnumItem_GetValue_String(item_.value(), &strValue);
            if (ANI_OK != status) {
                return status;
            }
            value = AniStringUtils::ToStd(env_, strValue);
            return ANI_OK;
        }

        expected<std::string, ani_status> ToString()
        {
            std::string value;
            ani_status status = ToString(value);
            if (ANI_OK != status) {
                return status;
            }
            return value;
        }

    private:
        ani_status GetItem(const char* className, ani_int index, ani_enum_item &item)
        {
            ani_status status = ANI_ERROR;
            ani_enum enumType;
            status = env_->FindEnum(className, &enumType);
            if (ANI_OK != status) {
                return status;
            }

            status = env_->Enum_GetEnumItemByIndex(enumType, index, &item);
            if (ANI_OK != status) {
                return status;
            }
            return ANI_OK;
        }

    private:
        ani_env *env_;
        std::optional<ani_enum_item> item_;
        ani_status initStatus_;
};
#endif
