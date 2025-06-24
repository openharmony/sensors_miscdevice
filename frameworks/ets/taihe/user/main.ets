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

import vibrator from "@ohos.vibrator";
import {BusinessError} from "@ohos.base";

loadLibrary("vibrator_taihe_native.z");

function testStopVibrator(){
    console.log("testStopVibrator begin");
    try {
        vibrator.stopVibrationSync();
    } catch (error) {
        let e: BusinessError = error as BusinessError;
        console.error(`stopVibrationSync failed.Code: ${e.code}, message: ${e.message}`);
    }
    console.log("testStopVibrator end");
}

function testStartPresetVibrator(){
    console.log("testStartPresetVibrator begin");
    try {
        let preset: vibrator.VibratePreset = {
            type: 'preset',
            effectId: 'haptic.effect.soft',
            count: 15,
            intensity: 50,
        };
        let attribute: vibrator.VibrateAttribute = {
            id: 1,
            usage: 'unknown',
        };
        vibrator.startVibration(preset, attribute, (error: BusinessError) => {
            console.info(`startVibration. Code: ${error.code}, message: ${error.message}`);
        });
    } catch (err) {
        let e: BusinessError = err as BusinessError;
        console.error(`An unexpected error occurred. Code: ${e.code}, message: ${e.message}`);
    }
    console.log("testStartPresetVibrator end");
}

function testStartTimeVibrator(){
    console.log("testStartTimeVibrator begin");
    try {
        let effect: vibrator.VibrateTime = {
            type: 'time',
            duration: 1000,
        };
        let attribute: vibrator.VibrateAttribute = {
            id: 1,
            usage: 'alarm',
        };
        vibrator.startVibration(effect, attribute, (error: BusinessError) => {
            console.info(`startVibration. Code: ${error.code}, message: ${error.message}`);
        });
    } catch (err) {
        let e: BusinessError = err as BusinessError;
        console.error(`An unexpected error occurred. Code: ${e.code}, message: ${e.message}`);
    }
    console.log("testStartTimeVibrator end");
}

function testIsSupportEffect(){
    console.log("testIsSupportEffect begin");
    vibrator.isSupportEffect('haptic.effect.soft', (error: BusinessError, state: boolean) => {
        console.info(`isSupportEffect. Code: ${error.code}, message: ${error.message}, state: ${state}`);
    });
    console.log("testIsSupportEffect end");
}

function testIsHdHapticSupported(){
    console.log("testIsHdHapticSupported begin");
    let ret = vibrator.isHdHapticSupported();
    console.log("testIsHdHapticSupported end " + ret);
}

function main(){
    testIsHdHapticSupported();
    testIsSupportEffect();
    testStartTimeVibrator();
    testStartPresetVibrator();
    testStopVibrator();
}