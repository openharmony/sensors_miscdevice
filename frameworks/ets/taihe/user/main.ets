import vibrator from "@ohos.vibrator";
import {BusinessError} from "@ohos.base";

loadLibrary("vibrator_taihe_native.z");

function testStopVibrator() {
    console.log("testStopVibrator begin");
    try {
        vibrator.stopVibrationSync();
    } catch (error) {
        let e: BusinessError = error as BusinessError;
        console.error(`stopVibrationSync failed.Code: ${e.code}, message: ${e.message}`);
    }
    console.log("testStopVibrator end");
}

function testStartPresetVibrator() {
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

function testStartTimeVibrator() {
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

function testIsSupportEffect() {
    console.log("testIsSupportEffect begin");
    vibrator.isSupportEffect('haptic.effect.soft', (error: BusinessError, state: boolean) => {
        console.info(`isSupportEffect. Code: ${error.code}, message: ${error.message}, state: ${state}`);
    });
    console.log("testIsSupportEffect end");
}

function testIsHdHapticSupported() {
    console.log("testIsHdHapticSupported begin");
    let ret = vibrator.isHdHapticSupported();
    console.log("testIsHdHapticSupported end " + ret);
}

function main() {
    testIsHdHapticSupported();
    testIsSupportEffect();
    testStartTimeVibrator();
    testStartPresetVibrator();
    testStopVibrator();
}