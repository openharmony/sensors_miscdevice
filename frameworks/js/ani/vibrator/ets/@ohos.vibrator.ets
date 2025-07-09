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

import type { AsyncCallback } from '@ohos.base';
import { BusinessError } from '@ohos.base';

export default namespace vibrator {
  loadLibrary("vibrator_ani");

  native function startVibrationSync(effect: VibrateEffect, attribute: VibrateAttribute): void;
  export function startVibrationIntSync(effect: VibrateEffect, attribute: VibrateAttribute): int {
    startVibrationSync(effect, attribute);
    return 0;
  }

  export function startVibration(effect: VibrateEffect, attribute: VibrateAttribute, callback: AsyncCallback<void>): void {
    let p1 = taskpool.execute(startVibrationIntSync, effect, attribute);
    p1.then((e: NullishType)=>{
        console.log("in callback startVibration then.")
        let err = new BusinessError<void>;
        callback(err, undefined);
    });
    p1.catch((err: NullishType) => {
      console.log("startVibration catch in callback thread.");
      let err2 = err as BusinessError<void>;
      callback(err2, undefined);
    })
  }

  export function startVibration(effect: VibrateEffect, attribute: VibrateAttribute): Promise<void> {
    let p = new Promise<void>((resolve: (v: undefined) => void, reject: (error: Object) => void) : void => {
      let p1 = taskpool.execute(startVibrationIntSync, effect, attribute);
      p1.then((e: NullishType) : void =>{
          console.log("in Promise startVibration then. e is ", e)
          resolve(undefined);
      });
      p1.catch((err: Error) : void => {
        console.log("startVibration catch in promise thread.");
        reject(err);
      });
    });
    return p;
  }

  export native function isSupportEffectInterally(effectId: string): boolean;

  export function isSupportEffect(effectId: string, callback: AsyncCallback<boolean>): void {
    let p1 = taskpool.execute(isSupportEffectInterally, effectId);
    p1.then((e: NullishType)=>{
        let r = e as boolean;
        console.log("in callback isSupportEffectInterally then. e is ", e)
        let err = new BusinessError<void>;
        callback(err, r);
    });
    p1.catch((error: NullishType) => {
      console.log("isSupportEffect catch in callback thread.");
      let err = error as BusinessError<void>;
      let e : boolean;
      callback(err, e);
    });
  }

  export function isSupportEffect(effectId: string): Promise<boolean> {
    let p = new Promise<boolean>((resolve: (v: boolean) => void, reject: (error: Object) => void) : boolean => {
      let p1 = taskpool.execute(isSupportEffectInterally, effectId);
      p1.then((e: NullishType) : boolean=>{
        let r = e as boolean;
        console.log("in promise isSupportEffectInterally then. e is ", e)
        resolve(r);
      });
      p1.catch((err: Error) : boolean => {
        console.log("isSupportEffect catch in promise thread.");
        reject(err);
      });
    });
    return p;
  }

  export type Usage = 'unknown' | 'alarm' | 'ring' | 'notification' | 'communication' |
  'touch' | 'media' | 'physicalFeedback' | 'simulateReality';

  export interface VibrateAttribute {
    id?: number;
    usage: Usage;
    systemUsage?: boolean;
  }

  export type VibrateEffect = VibrateTime | VibratePreset | VibrateFromFile | VibrateFromPattern;

  export interface VibrateTime {
    type: 'time';
    duration: number;
  }

  export interface VibratePreset {
    type: 'preset';
    effectId: string;
    count?: number;
    intensity?: number;
  }

  export interface VibrateFromFile {
    type: 'file';
    hapticFd: HapticFileDescriptor;
  }

  export interface HapticFileDescriptor {
    fd: number;
    offset?: number;
    length?: number;
  }

  export interface VibrateFromPattern {
    type: 'pattern';
    pattern: VibratorPattern;
  }

  export interface VibratorPattern {
    time: number;
    events: Array<VibratorEvent>;
  }

  export interface VibratorEvent {
    eventType: VibratorEventType;
    time: number;
    duration?: number;
    intensity?: number;
    frequency?: number;
    index?: number;
    points?: Array<VibratorCurvePoint>;
  }

  export enum VibratorEventType {
    CONTINUOUS = 0,
    TRANSIENT = 1,
  }

  export interface VibratorCurvePoint {
    time: number;
    intensity?: number;
    frequency?: number;
  }

}
