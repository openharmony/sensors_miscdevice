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

#include "vibrator_napi_utils.h"
#include <string>
#include "hilog/log.h"
#include "miscdevice_log.h"
#include "securec.h"

namespace OHOS {
namespace Sensors {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, MISC_LOG_DOMAIN, "VibratorJsAPI"};
}  // namespace
AsyncCallbackInfo::~AsyncCallbackInfo()
{
    CALL_LOG_ENTER;
    if (asyncWork != nullptr) {
        MISC_HILOGD("delete work");
        napi_delete_async_work(env, asyncWork);
    }
    for (int32_t i = 0; i < CALLBACK_NUM; ++i) {
        if (callback[i] != nullptr) {
            MISC_HILOGD("delete reference, i: %{public}d", i);
            napi_delete_reference(env, callback[i]);
        }
    }
}

bool IsMatchType(const napi_env &env, const napi_value &value, const napi_valuetype &type)
{
    napi_valuetype paramType = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &paramType), false);
    if (paramType != type) {
        MISC_HILOGE("Type mismatch");
        return false;
    }
    return true;
}

bool GetNapiInt32(const napi_env &env, const int32_t value, napi_value &result)
{
    CALL_LOG_ENTER;
    NAPI_CALL_BASE(env, napi_create_int32(env, value, &result), false);
    return true;
}

bool GetInt32Value(const napi_env &env, const napi_value &value, int32_t &result)
{
    CALL_LOG_ENTER;
    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &valuetype), false);
    NAPI_ASSERT_BASE(env, valuetype == napi_number,
        "Wrong argument type. Number or function expected", false);
    NAPI_CALL_BASE(env, napi_get_value_int32(env, value, &result), false);
    return true;
}

bool GetUint32Value(const napi_env &env, const napi_value &value, uint32_t &result)
{
    CALL_LOG_ENTER;
    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &valuetype), false);
    NAPI_ASSERT_BASE(env, valuetype == napi_number,
        "Wrong argument type. Number or function expected", false);
    NAPI_CALL_BASE(env, napi_get_value_uint32(env, value, &result), false);
    return true;
}

bool GetInt64Value(const napi_env &env, const napi_value &value, int64_t &result)
{
    CALL_LOG_ENTER;
    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &valuetype), false);
    NAPI_ASSERT_BASE(env, valuetype == napi_number,
        "Wrong argument type. Number or function expected", false);
    NAPI_CALL_BASE(env, napi_get_value_int64(env, value, &result), false);
    return true;
}

bool GetStringValue(const napi_env &env, const napi_value &value, string &result)
{
    CALL_LOG_ENTER;
    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &valuetype), false);
    NAPI_ASSERT_BASE(env, (valuetype == napi_string), "Wrong argument type. String or function expected", false);
    size_t bufLength = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength), false);
    bufLength = bufLength > STRING_LENGTH_MAX ? STRING_LENGTH_MAX : bufLength;
    char str[STRING_LENGTH_MAX] = {0};
    size_t strLen = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, str, bufLength + 1, &strLen), false);
    result = str;
    return true;
}

bool GetPropertyString(const napi_env &env, const napi_value &value, const std::string &type, std::string &result)
{
    bool exist = false;
    napi_status status = napi_has_named_property(env, value, type.c_str(), &exist);
    if ((status != napi_ok) || (!exist)) {
        MISC_HILOGE("can not find %{public}s property", type.c_str());
        napi_throw_error(env, nullptr, "can not find property of string");
        return false;
    }

    napi_value item = nullptr;
    CHKNRF(env, napi_get_named_property(env, value, type.c_str(), &item), napi_ok, "napi get property fail");
    if (!GetStringValue(env, item, result)) {
        return false;
    }
    return true;
}

bool GetPropertyInt32(const napi_env &env, const napi_value &value, const std::string &type, int32_t &result)
{
    napi_value item = nullptr;
    bool exist = false;
    napi_status status = napi_has_named_property(env, value, type.c_str(), &exist);
    if (status != napi_ok || !exist) {
        MISC_HILOGE("can not find %{public}s property", type.c_str());
        napi_throw_error(env, nullptr, "can not find property of int");
        return false;
    }
    CHKNRF(env, napi_get_named_property(env, value, type.c_str(), &item), napi_ok, "napi get property fail");
    if (!GetInt32Value(env, item, result)) {
        MISC_HILOGE("Get int value fail");
        return false;
    }
    return true;
}

napi_value GreateCallbackError(const napi_env &env, const int32_t errCode,
    const string errMessage, const string errName, const string errStack)
{
    napi_value code = nullptr;
    NAPI_CALL(env, napi_create_int32(env, errCode, &code));
    napi_value message = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, errMessage.data(), NAPI_AUTO_LENGTH, &message));
    napi_value name = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, errName.data(), NAPI_AUTO_LENGTH, &name));
    napi_value stack = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, errStack.data(), NAPI_AUTO_LENGTH, &stack));
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    NAPI_CALL(env, napi_set_named_property(env, result, "code", code));
    NAPI_CALL(env, napi_set_named_property(env, result, "message", message));
    NAPI_CALL(env, napi_set_named_property(env, result, "name", name));
    NAPI_CALL(env, napi_set_named_property(env, result, "stack", stack));
    return result;
}

void emitSystemCallback(const napi_env &env, sptr<AsyncCallbackInfo> asyncCallbackInfo)
{
    CHKPV(asyncCallbackInfo);
    if (asyncCallbackInfo->error.code == SUCCESS) {
        CHKPV(asyncCallbackInfo->callback[0]);
        napi_value callback = nullptr;
        NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, asyncCallbackInfo->callback[0], &callback));
        napi_value result = nullptr;
        NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &result));
        napi_value callResult = nullptr;
        NAPI_CALL_RETURN_VOID(env, napi_call_function(env, nullptr, callback, 1, &result, &callResult));
        return;
    }
    CHKPV(asyncCallbackInfo->callback[1]);
    napi_value callback = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, asyncCallbackInfo->callback[1], &callback));
    napi_value result[2] = {0};
    NAPI_CALL_RETURN_VOID(env, napi_create_string_utf8(env, asyncCallbackInfo->error.message.data(),
        NAPI_AUTO_LENGTH, &result[0]));
    NAPI_CALL_RETURN_VOID(env, napi_create_int32(env, asyncCallbackInfo->error.code, &result[1]));
    napi_value callResult = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, nullptr, callback, 1, result, &callResult));
}

void EmitAsyncCallbackWork(sptr<AsyncCallbackInfo> asyncCallbackInfo)
{
    CALL_LOG_ENTER;
    CHKPV(asyncCallbackInfo);
    CHKPV(asyncCallbackInfo->env);
    napi_env env = asyncCallbackInfo->env;
    napi_value resourceName = nullptr;
    NAPI_CALL_RETURN_VOID(env,
        napi_create_string_latin1(env, "AsyncCallback", NAPI_AUTO_LENGTH, &resourceName));
    asyncCallbackInfo->IncStrongRef(nullptr);
    napi_status status = napi_create_async_work(
        env, nullptr, resourceName, [](napi_env env, void* data) {},
        [](napi_env env, napi_status status, void* data) {
            CALL_LOG_ENTER;
            sptr<AsyncCallbackInfo> asyncCallbackInfo(static_cast<AsyncCallbackInfo *>(data));
            /**
             * After the asynchronous task is created, the asyncCallbackInfo reference count is reduced
             * to 0 destructions, so you need to add 1 to the asyncCallbackInfo reference count when the
             * asynchronous task is created, and subtract 1 from the reference count after the naked
             * pointer is converted to a pointer when the asynchronous task is executed, the reference
             * count of the smart pointer is guaranteed to be 1.
             */
            asyncCallbackInfo->DecStrongRef(nullptr);
            if (asyncCallbackInfo->callbackType == TYPE_SYSTEM_VIBRATE) {
                emitSystemCallback(env, asyncCallbackInfo);
                return;
            }
            CHKPV(asyncCallbackInfo->callback[0]);
            napi_value callback = nullptr;
            NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, asyncCallbackInfo->callback[0], &callback));
            napi_value result = nullptr;
            if (asyncCallbackInfo->error.code < 0) {
                result = GreateCallbackError(env, asyncCallbackInfo->error.code, asyncCallbackInfo->error.message,
                    asyncCallbackInfo->error.name, asyncCallbackInfo->error.stack);
                CHKPV(result);
            } else {
                NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &result));
            }
            napi_value callResult = nullptr;
            NAPI_CALL_RETURN_VOID(env, napi_call_function(env, nullptr, callback, 1, &result, &callResult));
        },
        asyncCallbackInfo.GetRefPtr(), &asyncCallbackInfo->asyncWork);
    if (status != napi_ok
        || napi_queue_async_work(asyncCallbackInfo->env, asyncCallbackInfo->asyncWork) != napi_ok) {
        MISC_HILOGE("Create async work fail");
        asyncCallbackInfo->DecStrongRef(nullptr);
    }
}

void EmitPromiseWork(sptr<AsyncCallbackInfo> asyncCallbackInfo)
{
    CALL_LOG_ENTER;
    CHKPV(asyncCallbackInfo);
    CHKPV(asyncCallbackInfo->env);
    napi_env env = asyncCallbackInfo->env;
    napi_value resourceName = nullptr;
    NAPI_CALL_RETURN_VOID(env,
        napi_create_string_latin1(asyncCallbackInfo->env, "Promise", NAPI_AUTO_LENGTH, &resourceName));
    // Make the reference count of asyncCallbackInfo add 1, and the function exits the non-destructor
    asyncCallbackInfo->IncStrongRef(nullptr);
    napi_status status = napi_create_async_work(
        asyncCallbackInfo->env, nullptr, resourceName, [](napi_env env, void* data) {},
        [](napi_env env, napi_status status, void* data) {
            CALL_LOG_ENTER;
            sptr<AsyncCallbackInfo> asyncCallbackInfo(static_cast<AsyncCallbackInfo *>(data));
            /**
             * After the asynchronous task is created, the asyncCallbackInfo reference count is reduced
             * to 0 destructions, so you need to add 1 to the asyncCallbackInfo reference count when the
             * asynchronous task is created, and subtract 1 from the reference count after the naked
             * pointer is converted to a pointer when the asynchronous task is executed, the reference
             * count of the smart pointer is guaranteed to be 1.
             */
            asyncCallbackInfo->DecStrongRef(nullptr);
            CHKPV(asyncCallbackInfo->deferred);
            if (asyncCallbackInfo->callbackType == TYPE_SYSTEM_VIBRATE) {
                emitSystemCallback(env, asyncCallbackInfo);
                return;
            }
            napi_value result = nullptr;
            if (asyncCallbackInfo->error.code == 0) {
                NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &result));
                NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, asyncCallbackInfo->deferred, result));
            } else {
                result = GreateCallbackError(env, asyncCallbackInfo->error.code, asyncCallbackInfo->error.message,
                    asyncCallbackInfo->error.name, asyncCallbackInfo->error.stack);
                CHKPV(result);
                NAPI_CALL_RETURN_VOID(env, napi_reject_deferred(env, asyncCallbackInfo->deferred, result));
            }
        }, asyncCallbackInfo.GetRefPtr(), &asyncCallbackInfo->asyncWork);
    if (status != napi_ok
        || napi_queue_async_work(asyncCallbackInfo->env, asyncCallbackInfo->asyncWork) != napi_ok) {
        MISC_HILOGE("Create async work fail");
        asyncCallbackInfo->DecStrongRef(nullptr);
    }
}
}  // namespace Sensors
}  // namespace OHOS
