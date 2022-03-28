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

#include <cstdio>
#include <map>
#include <string>
#include <unistd.h>

#include "hilog/log.h"
#include "miscdevice_log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "vibrator_agent.h"
#include "vibrator_napi_utils.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
static constexpr HiLogLabel LABEL = { LOG_CORE, 0xD002757, "VibratorJsAPI" };
static constexpr uint32_t VIBRATE_SHORT_DURATION = 35;
static constexpr uint32_t VIBRATE_LONG_DURATION = 1000;

static napi_value VibrateTime(napi_env env, napi_value args[], size_t argc)
{
    NAPI_ASSERT(env, (argc == 1 || argc == 2), "Wrong argument number");
    int32_t duration = 0;
    NAPI_ASSERT(env, GetInt32Value(env, args[0], duration), "Get int number fail");
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env);
    CHKPP(asyncCallbackInfo);
    if (duration < 0) {
        MISC_HILOGE("Vibrate duration is invalid, duration: %{public}d", duration);
        asyncCallbackInfo->error.code = ERROR;
        asyncCallbackInfo->error.message = "Vibrate duration is invalid";
    } else {
        asyncCallbackInfo->error.code = StartVibratorOnce(static_cast<uint32_t>(duration));
    }

    if (argc == 2) {
        NAPI_ASSERT(env, IsMatchType(env, args[1], napi_function), "Wrong argument type. function expected");
        NAPI_CALL(env, napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]));
        EmitAsyncCallbackWork(asyncCallbackInfo);
        return nullptr;
    }

    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    asyncCallbackInfo->deferred = deferred;
    EmitPromiseWork(asyncCallbackInfo);
    return promise;
}

static napi_value VibrateEffectId(napi_env env, napi_value args[], size_t argc)
{
    NAPI_ASSERT(env, (argc == 1 || argc == 2), "Wrong argument number");
    string effectId;
    NAPI_ASSERT(env, GetStringValue(env, args[0], effectId), "Wrong argument type. String or function expected");
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env);
    CHKPP(asyncCallbackInfo);
    asyncCallbackInfo->error.code = StartVibrator(effectId.c_str());

    if (argc == 2) {
        NAPI_ASSERT(env, IsMatchType(env, args[1], napi_function), "Wrong argument type. function expected");
        NAPI_CALL(env, napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]));
        EmitAsyncCallbackWork(asyncCallbackInfo);
        return nullptr;
    }

    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    asyncCallbackInfo->deferred = deferred;
    EmitPromiseWork(asyncCallbackInfo);
    return promise;
}

static bool GetCallbackInfo(const napi_env &env, napi_value args[],
    sptr<AsyncCallbackInfo> &asyncCallbackInfo, string &mode)
{
    CHKPF(asyncCallbackInfo);
    CHKPF(args);
    napi_value value = nullptr;
    NAPI_CALL_BASE(env, napi_get_named_property(env, args[0], "success", &value), false);
    NAPI_CALL_BASE(env, napi_create_reference(env, value, 1, &asyncCallbackInfo->callback[0]), false);

    bool result = false;
    NAPI_CALL_BASE(env, napi_has_named_property(env, args[0], "mode", &result), false);
    if (result) {
        NAPI_CALL_BASE(env, napi_get_named_property(env, args[0], "mode", &value), false);
        NAPI_ASSERT_BASE(env, GetStringValue(env, value, mode),
            "Wrong argument type. String or function expected", false);
        NAPI_ASSERT_BASE(env, (mode == "long" || mode == "short"),
            "Wrong argument type. Invalid mode value", false);
    }
    NAPI_CALL_BASE(env, napi_has_named_property(env, args[0], "fail", &result), false);
    if (result) {
        NAPI_CALL_BASE(env, napi_get_named_property(env, args[0], "fail", &value), false);
        NAPI_CALL_BASE(env, napi_create_reference(env, value, 1, &asyncCallbackInfo->callback[1]), false);
    }
    NAPI_CALL_BASE(env, napi_has_named_property(env, args[0], "complete", &result), false);
    if (result) {
        NAPI_CALL_BASE(env, napi_get_named_property(env, args[0], "complete", &value), false);
        NAPI_CALL_BASE(env, napi_create_reference(env, value, 1, &asyncCallbackInfo->callback[2]), false);
    }
    return true;
}

static napi_value VibrateMode(napi_env env, napi_value args[], size_t argc)
{
    if (argc == 0) {
        NAPI_ASSERT(env, (StartVibratorOnce(VIBRATE_LONG_DURATION) == 0), "Vibrate long mode fail");
        return nullptr;
    }
    NAPI_ASSERT(env, (argc == 1), "Param number is invalid");
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env);
    CHKPP(asyncCallbackInfo);
    asyncCallbackInfo->callbackType = TYPE_SYSTEM_VIBRATE;
    string mode = "long";
    NAPI_ASSERT(env, GetCallbackInfo(env, args, asyncCallbackInfo, mode), "Get callback info fail");
    uint32_t duration = ((mode == "long") ? VIBRATE_LONG_DURATION : VIBRATE_SHORT_DURATION);
    asyncCallbackInfo->error.code = StartVibratorOnce(duration);
    if (asyncCallbackInfo->error.code != SUCCESS) {
        asyncCallbackInfo->error.message = "Vibrator vibrate fail";
    }
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value Vibrate(napi_env env, napi_callback_info info)
{
    CHKPP(env);
    CHKPP(info);
    size_t argc = 2;
    napi_value args[2] = {};
    napi_value thisArg = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisArg, nullptr));
    NAPI_ASSERT(env, (argc <= 2), "Wrong argument number");

    if (IsMatchType(env, args[0], napi_number)) {
        return VibrateTime(env, args, argc);
    }
    if (IsMatchType(env, args[0], napi_string)) {
        return VibrateEffectId(env, args, argc);
    }
    if (IsMatchType(env, args[0], napi_object)) {
        return VibrateMode(env, args, argc);
    }
    NAPI_CALL(env, napi_throw_error((env), nullptr, "Wrong argument type"));
    return nullptr;
}

static napi_value Stop(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = {};
    napi_value thisArg = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisArg, nullptr));
    NAPI_ASSERT(env, (argc == 1 || argc == 2), "Wrong argument number");
    string mode;
    NAPI_ASSERT(env, GetStringValue(env, args[0], mode), "Wrong argument type. String or function expected");
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env);
    CHKPP(asyncCallbackInfo);
    asyncCallbackInfo->error.code = StopVibrator(mode.c_str());
    if (argc == 2) {
        NAPI_ASSERT(env, IsMatchType(env, args[1], napi_function), "Wrong argument type. function expected");
        NAPI_CALL(env, napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]));
        EmitAsyncCallbackWork(asyncCallbackInfo);
        return nullptr;
    }
    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    asyncCallbackInfo->deferred = deferred;
    EmitPromiseWork(asyncCallbackInfo);
    return promise;
}

static napi_value EnumClassConstructor(const napi_env env, const napi_callback_info info)
{
    size_t argc = 0;
    napi_value args[1] = {0};
    napi_value res = nullptr;
    void *data = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &res, &data));
    return res;
}

static napi_value CreateEnumStopMode(const napi_env env, napi_value exports)
{
    napi_value timeMode = nullptr;
    napi_value presetMode = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "time", NAPI_AUTO_LENGTH, &timeMode));
    NAPI_CALL(env, napi_create_string_utf8(env, "preset", NAPI_AUTO_LENGTH, &presetMode));

    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("VIBRATOR_STOP_MODE_TIME", timeMode),
        DECLARE_NAPI_STATIC_PROPERTY("VIBRATOR_STOP_MODE_PRESET", presetMode),
    };
    napi_value result = nullptr;
    NAPI_CALL(env, napi_define_class(env, "VibratorStopMode", NAPI_AUTO_LENGTH, EnumClassConstructor, nullptr,
        sizeof(desc) / sizeof(*desc), desc, &result));
    NAPI_CALL(env, napi_set_named_property(env, exports, "VibratorStopMode", result));
    return exports;
}

static napi_value CreateEnumEffectId(const napi_env env, const napi_value exports)
{
    napi_value clockTime = nullptr;
    napi_create_string_utf8(env, "haptic.clock.timer", NAPI_AUTO_LENGTH, &clockTime);

    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("EFFECT_CLOCK_TIMER", clockTime),
    };
    napi_value result = nullptr;
    NAPI_CALL(env, napi_define_class(env, "EffectId", NAPI_AUTO_LENGTH, EnumClassConstructor, nullptr,
        sizeof(desc) / sizeof(*desc), desc, &result));
    NAPI_CALL(env, napi_set_named_property(env, exports, "EffectId", result));
    return exports;
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("vibrate", Vibrate),
        DECLARE_NAPI_FUNCTION("stop", Stop)
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(napi_property_descriptor), desc));
    NAPI_ASSERT_BASE(env, CreateEnumStopMode(env, exports) != nullptr, "Create enum stop mode fail", exports);
    NAPI_ASSERT_BASE(env, CreateEnumEffectId(env, exports) != nullptr, "Create enum effect id fail", exports);
    return exports;
}

static napi_module _module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = NULL,
    .nm_register_func = Init,
    .nm_modname = "vibrator",
    .nm_priv = ((void *)0),
    .reserved = {0}
};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&_module);
}
}  // namespace Sensors
}  // namespace OHOS
