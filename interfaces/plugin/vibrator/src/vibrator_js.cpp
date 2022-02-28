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
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "vibrator_agent.h"
#include "vibrator_napi_utils.h"

using namespace OHOS::HiviewDFX;
static constexpr HiLogLabel LABEL = { LOG_CORE, 0xD002757, "VibratorJsAPI" };


static napi_value Vibrate(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[CALLBACK_RESULT_LENGTH];
    napi_value thisArg;
    napi_get_cb_info(env, info, &argc, args, &thisArg, nullptr);
    if (argc < 1 || argc > CALLBACK_RESULT_LENGTH) {
        HiLog::Error(LABEL, "%{public}s the number of input parameters does not match", __func__);
        return nullptr;
    }
    if (!IsMatchType(args[0], napi_number, env) && !IsMatchType(args[0], napi_string, env)) {
        HiLog::Error(LABEL, "%{public}s input parameter type does not match number or string", __func__);
        return nullptr;
    }
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
    };
    if (IsMatchType(args[0], napi_number, env)) {
        int32_t duration = GetCppInt32(args[0], env);
        asyncCallbackInfo->error.code = StartVibratorOnce(duration);
    } else if (IsMatchType(args[0], napi_string, env)) {
        size_t bufLength = 0;
        napi_status status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &bufLength);
        if (bufLength < 0) {
            HiLog::Error(LABEL, "%{public}s input parameter is invalid", __func__);
            return nullptr;
        }
        char *vibratorEffect = (char *)malloc((bufLength + 1) * sizeof(char));
        status = napi_get_value_string_utf8(env, args[0], vibratorEffect, bufLength + 1, &bufLength);
        asyncCallbackInfo->error.code = StartVibrator(vibratorEffect);
        if (vibratorEffect != nullptr) {
            free(vibratorEffect);
        }
    }
    if (argc == CALLBACK_RESULT_LENGTH) {
        if (!IsMatchType(args[1], napi_function, env)) {
            HiLog::Error(LABEL, "%{public}s input parameter type does not match function", __func__);
            delete asyncCallbackInfo;
            asyncCallbackInfo = nullptr;
            return nullptr;
        }
        napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]);
        EmitAsyncCallbackWork(asyncCallbackInfo);
    } else if (argc == 1) {
        napi_deferred deferred;
        napi_value promise;
        napi_create_promise(env, &deferred, &promise);
        asyncCallbackInfo->deferred = deferred;
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    return nullptr;
}

static napi_value Stop(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_value thisArg;
    napi_get_cb_info(env, info, &argc, args, &thisArg, nullptr);
    if (argc < 1 || argc > CALLBACK_RESULT_LENGTH) {
        HiLog::Error(LABEL, "%{public}s the number of input parameters does not match", __func__);
        return nullptr;
    }
    if (!IsMatchType(args[0], napi_string, env)) {
        HiLog::Error(LABEL, "%{public}s input parameter type does not match string", __func__);
        return nullptr;
    }
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
    };
    size_t bufLength = 0;
    napi_status status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &bufLength);
    if (bufLength < 0) {
        HiLog::Error(LABEL, "%{public}s input parameter is invalid", __func__);
        return nullptr;
    }
    char *mode = (char *)malloc((bufLength + 1) * sizeof(char));
    status = napi_get_value_string_utf8(env, args[0], mode, bufLength + 1, &bufLength);
    asyncCallbackInfo->error.code = StopVibrator(mode);
    if (mode != nullptr) {
        free(mode);
    }
    if (argc == CALLBACK_RESULT_LENGTH) {
        if (!IsMatchType(args[1], napi_function, env) || !IsMatchType(args[0], napi_string, env)) {
            HiLog::Error(LABEL, "%{public}s input parameter type does not match", __func__);
            delete asyncCallbackInfo;
            asyncCallbackInfo = nullptr;
            return nullptr;
        }
        napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]);
        EmitAsyncCallbackWork(asyncCallbackInfo);
    } else if (argc == 1) {
        napi_deferred deferred;
        napi_value promise;
        napi_create_promise(env, &deferred, &promise);
        asyncCallbackInfo->deferred = deferred;
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    return nullptr;
}

static napi_value EnumClassConstructor(const napi_env env, const napi_callback_info info)
{
    size_t argc = 0;
    napi_value args[1] = {0};
    napi_value res = nullptr;
    void *data = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &res, &data);
    if (status != napi_ok) {
        return nullptr;
    }
    return res;
}

static napi_value CreateEnumStopMode(const napi_env env, napi_value exports)
{
    napi_value timeMode = nullptr;
    napi_value presetMode = nullptr;
    napi_create_string_utf8(env, "time", NAPI_AUTO_LENGTH, &timeMode);
    napi_create_string_utf8(env, "preset", NAPI_AUTO_LENGTH, &presetMode);

    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("VIBRATOR_STOP_MODE_TIME", timeMode),
        DECLARE_NAPI_STATIC_PROPERTY("VIBRATOR_STOP_MODE_PRESET", presetMode),
    };
    napi_value result = nullptr;
    napi_define_class(env, "VibratorStopMode", NAPI_AUTO_LENGTH, EnumClassConstructor, nullptr,
        sizeof(desc) / sizeof(*desc), desc, &result);
    napi_set_named_property(env, exports, "VibratorStopMode", result);
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
    napi_define_class(env, "EffectId", NAPI_AUTO_LENGTH, EnumClassConstructor, nullptr,
        sizeof(desc) / sizeof(*desc), desc, &result);
    napi_set_named_property(env, exports, "EffectId", result);
    return exports;
}

EXTERN_C_START

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("vibrate", Vibrate),
        DECLARE_NAPI_FUNCTION("stop", Stop)
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(napi_property_descriptor), desc));
    CreateEnumStopMode(env, exports);
    CreateEnumEffectId(env, exports);
    return exports;
}
EXTERN_C_END

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
