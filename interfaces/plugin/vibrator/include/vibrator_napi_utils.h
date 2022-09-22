/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef VIBRATOR_NAPI_UTILS_H
#define VIBRATOR_NAPI_UTILS_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include <cstring>
#include <iostream>
#include "refbase.h"

namespace OHOS {
namespace Sensors {
using std::string;
constexpr int32_t CALLBACK_NUM = 3;
constexpr uint32_t TYPE_SYSTEM_VIBRATE = 1;
constexpr uint32_t STRING_LENGTH_MAX = 64;

class AsyncCallbackInfo : public RefBase {
public:
    struct AsyncCallbackError {
        int32_t code {0};
        string message;
        string name;
        string stack;
    };

    napi_env env = nullptr;
    napi_async_work asyncWork = nullptr;
    napi_deferred deferred = nullptr;
    napi_ref callback[CALLBACK_NUM] = {0};
    AsyncCallbackError error;
    uint32_t callbackType = 0;
    AsyncCallbackInfo(napi_env env) : env(env) {}
    ~AsyncCallbackInfo();
};

bool IsMatchType(const napi_env &env, const napi_value &value, const napi_valuetype &type);
bool GetNapiInt32(const napi_env &env, const int32_t value, napi_value &result);
bool GetInt32Value(const napi_env &env, const napi_value &value, int32_t &result);
bool GetUint32Value(const napi_env &env, const napi_value &value, uint32_t &result);
bool GetStringValue(const napi_env &env, const napi_value &value, string &result);
bool GetPropertyString(const napi_env &env, const napi_value &value, const std::string &type, std::string &result);
bool GetPropertyInt32(const napi_env &env, const napi_value &value, const std::string &type, int32_t &result);
bool GetNapiParam(const napi_env &env, const napi_callback_info &info, size_t &argc, napi_value &argv);
void EmitAsyncCallbackWork(sptr<AsyncCallbackInfo> async_callback_info);
bool GetInt64Value(const napi_env &env, const napi_value &value, int32_t &result);
void EmitPromiseWork(sptr<AsyncCallbackInfo> asyncCallbackInfo);
napi_value GreateCallbackError(const napi_env &env, const int32_t errCode,
    const string errMessage, const string errName, const string errStack);

#define CHKNRR(env, state, message, retVal) \
    do { \
        if ((state) != napi_ok) { \
            MISC_HILOGE("(%{public}s) fail", #message); \
            auto errDesc = std::string(__FUNCTION__) + ": " + #message; \
            napi_throw_error(env, nullptr, errDesc.c_str()); \
            return retVal; \
        } \
    } while (0)

#define CHKNCP(env, cond, message) \
    do { \
        if (!(cond)) { \
            MISC_HILOGE("(%{public}s)", #message); \
            auto errDesc = std::string(__FUNCTION__) + ": " + #message; \
            napi_throw_error(env, nullptr, errDesc.c_str()); \
            return nullptr; \
        } \
    } while (0)

#define CHKNRF(env, state, ret, message) \
    do { \
        if ((state) != (ret)) { \
            MISC_HILOGE("(%{public}s)", #message); \
            auto errDesc = std::string(__FUNCTION__) + ": " + #message; \
            napi_throw_error(env, nullptr, errDesc.c_str()); \
            return false; \
        } \
    } while (0)
}  // namespace Sensors
}  // namespace OHOS
#endif // VIBRATOR_NAPI_UTILS_H