/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
import vibrator from '@ohos.vibrator';
import resourceManager from '@ohos.resourceManager';
import fileio from '@ohos.fileio';

import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect, TestType, Size, Level } from '@ohos/hypium'

export default function CustomHapticJsTest_misc_1() {
    describe("CustomHapticJsTest_misc_1", function () {
        const DEVICE_OPERATION_FAILED_CODE = 14600101;
        const PERMISSION_DENIED_CODE = 201;
        const PARAMETER_ERROR_CODE = 401;
        const IS_NOT_SUPPORTED_CODE = 801;

        const DEVICE_OPERATION_FAILED_MSG = 'Device operation failed.'
        const PERMISSION_DENIED_MSG = 'Permission denied.'
        const PARAMETER_ERROR_MSG = 'The parameter invalid.'
        const IS_NOT_SUPPORTED_MSG = 'Capability not supported.'

        const FILE_PATH = "/data/storage/el2/base/files/"
        const FILE_NAME = "coin_drop.json"

        beforeAll(function () {

            /*
             * @tc.setup: setup invoked before all testcases
             */
            console.info('beforeAll called')
        })

        afterAll(function () {

            /*
             * @tc.teardown: teardown invoked after all testcases
             */
            console.info('afterAll called')
        })

        beforeEach(function () {

            /*
             * @tc.setup: setup invoked before each testcases
             */
            console.info('beforeEach called')
        })

        afterEach(function () {

            /*
             * @tc.teardown: teardown invoked after each testcases
             */
            console.info('afterEach called')
            vibrator.stopVibration();
            console.info('afterEach called')
        })

        async function openResource(fileName) {
            let fileDescriptor = undefined;
            let mgr = await resourceManager.getResourceManager();
            await mgr.getRawFd(fileName).then(value => {
                fileDescriptor = {fd: value.fd, offset: value.offset, length: value.length};
                console.log('case openResource success fileName: ' + fileName);
            }).catch(error => {
                console.log('case openResource err: ' + error);
            });
            return fileDescriptor;
        }

        async function closeResource(fileName) {
            let mgr = await resourceManager.getResourceManager();
            await mgr.closeRawFd(fileName).then(()=> {
                console.log('case closeResource success fileName: ' + fileName);
            }).catch(error => {
                console.log('case closeResource err: ' + error);
            });
        }

        async function openFile(fileName) {
            let fd = undefined;
            await fileio.open(FILE_PATH + fileName).then(value => {
                fd = value;
                console.log('case openFile success fileName: ' + fileName);
            }).catch(error => {
                console.log('case openFile err: ' + error);
            });
            return fd;
        }

        async function closeFile(fd) {
            await fileio.close(fd).then(()=> {
                console.log('case closeFile success fd: ' + fd);
            }).catch(error => {
                console.log('case closeFile err: ' + error);
            });
        }

        /*
         * @tc.name:CustomHapticJsTest
         * @tc.desc:Verification results of the incorrect parameters of the test interface.
         * @tc.number:CustomHapticJsTest001
         */
        it("CustomHapticJsTest001", 0, async function (done) {
            let rawFd = await openResource(FILE_NAME);
            await vibrator.startVibration({
                type: "file",
                hapticFd: { fd: rawFd.fd, offset: rawFd.offset, length: rawFd.length }
            }, {
                usage: "unknown"
            }, (error)=>{
                if (error) {
                    console.info('CustomHapticJsTest001 error');
                    expect(error.code).assertEqual(IS_NOT_SUPPORTED_CODE);
                    expect(error.message).assertEqual(IS_NOT_SUPPORTED_MSG);
                } else {
                    console.info('CustomHapticJsTest001 success');
                    expect(true).assertTrue();
                }
                done();
            });
            await closeResource(FILE_NAME);
            done();
        })

        /*
         * @tc.name:CustomHapticJsTest
         * @tc.desc:Verification results of the incorrect parameters of the test interface.
         * @tc.number:CustomHapticJsTest002
         */
        it("CustomHapticJsTest002", 0, async function (done) {
            let rawFd = await openResource(FILE_NAME);
            await vibrator.startVibration({
                type: "file",
                hapticFd: { fd: rawFd.fd, offset: rawFd.offset, length: rawFd.length }
            }, {
                usage: "unknown"
            }).then(() => {
                console.info('CustomHapticJsTest002 success');
                expect(true).assertTrue();
            }, (error) => {
                console.info('CustomHapticJsTest002 error');
                expect(error.code).assertEqual(IS_NOT_SUPPORTED_CODE);
                expect(error.message).assertEqual(IS_NOT_SUPPORTED_MSG);
            });
            await closeResource(FILE_NAME);
            done();
        })

        /*
         * @tc.name:CustomHapticJsTest
         * @tc.desc:Verification results of the incorrect parameters of the test interface.
         * @tc.number:CustomHapticJsTest003
         */
        it("CustomHapticJsTest003", 0, async function (done) {
            let fileFd = await openFile(FILE_NAME);
            await vibrator.startVibration({
                type: "file",
                hapticFd: { fd: fileFd }
            }, {
                usage: "unknown"
            }).then(() => {
                console.info('CustomHapticJsTest003 success');
                expect(true).assertTrue();
            }, (error) => {
                console.info('CustomHapticJsTest003 error');
                expect(error.code).assertEqual(IS_NOT_SUPPORTED_CODE);
                expect(error.message).assertEqual(IS_NOT_SUPPORTED_MSG);
            });
            await closeFile(fileFd);
            done();
        })

        /*
         * @tc.name:CustomHapticJsTest
         * @tc.desc:Verification results of the incorrect parameters of the test interface.
         * @tc.number:CustomHapticJsTest004
         */
        it("CustomHapticJsTest004", 0, async function (done) {
            let fileFd = await openFile(FILE_NAME);
            await vibrator.startVibration({
                type: "file",
                hapticFd: { fd: fileFd, offset: 0, length: -1 }
            }, {
                usage: "unknown"
            }).then(() => {
                console.info('CustomHapticJsTest004 success');
                expect(true).assertTrue();
            }, (error) => {
                console.info('CustomHapticJsTest004 error');
                expect(error.code).assertEqual(IS_NOT_SUPPORTED_CODE);
                expect(error.message).assertEqual(IS_NOT_SUPPORTED_MSG);
            });
            await closeFile(fileFd);
            done();
        })

        /*
         * @tc.name:CustomHapticJsTest
         * @tc.desc:Verification results of the incorrect parameters of the test interface.
         * @tc.number:CustomHapticJsTest005
         */
        it("CustomHapticJsTest005", 0, async function (done) {
            let fd = await openFile(FILE_NAME);
            await vibrator.startVibration({
                type: "file",
                hapticFd: { fd: fd, offset: undefined, length: undefined }
            }, {
                usage: "unknown"
            }).then(() => {
                console.info('CustomHapticJsTest005 success');
                expect(true).assertTrue();
            }, (error) => {
                console.info('CustomHapticJsTest005 error');
                expect(error.code).assertEqual(IS_NOT_SUPPORTED_CODE);
                expect(error.message).assertEqual(IS_NOT_SUPPORTED_MSG);
            });
            await closeFile(fd);
            done();
        })

        /*
         * @tc.name:CustomHapticJsTest
         * @tc.desc:Verification results of the incorrect parameters of the test interface.
         * @tc.number:CustomHapticJsTest006
         */
        it("CustomHapticJsTest006", 0, async function (done) {
            let fd = await openFile(FILE_NAME);
            await vibrator.startVibration({
                type: "file",
                hapticFd: { fd: fd, offset: null, length: null }
            }, {
                usage: "unknown"
            }).then(() => {
                console.info('CustomHapticJsTest006 success');
                expect(true).assertTrue();
            }, (error) => {
                console.info('CustomHapticJsTest006 error');
                expect(error.code).assertEqual(IS_NOT_SUPPORTED_CODE);
                expect(error.message).assertEqual(IS_NOT_SUPPORTED_MSG);
            });
            await closeFile(fd);
        })

        /*
         * @tc.name:CustomHapticJsTest
         * @tc.desc:Verification results of the incorrect parameters of the test interface.
         * @tc.number:CustomHapticJsTest007
         */
        it("CustomHapticJsTest007", 0, async function (done) {
            let fd = await openFile(FILE_NAME);
            await vibrator.startVibration({
                type: "file",
                hapticFd: { fd: fd, offset: "str", length: "str" }
            }, {
                usage: "unknown"
            }).then(() => {
                console.info('CustomHapticJsTest007 success');
                expect(true).assertTrue();
            }, (error) => {
                console.info('CustomHapticJsTest007 error');
                expect(error.code).assertEqual(IS_NOT_SUPPORTED_CODE);
                expect(error.message).assertEqual(IS_NOT_SUPPORTED_MSG);
            });
            await closeFile(fd);
            done();
        })

        /*
         * @tc.name:CustomHapticJsTest
         * @tc.desc:Verification results of the incorrect parameters of the test interface.
         * @tc.number:CustomHapticJsTest008
         */
        it("CustomHapticJsTest008", 0, async function (done) {
            await vibrator.startVibration({
                type: "file",
                hapticFd: { fd: "" }
            }, {
                usage: "unknown"
            }).then(() => {
                console.info('CustomHapticJsTest008 success');
                expect(false).assertTrue();
            }, (error) => {
                console.info('CustomHapticJsTest008 error');
                expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
                expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            });
            done();
        })

        /*
         * @tc.name:CustomHapticJsTest
         * @tc.desc:Verification results of the incorrect parameters of the test interface.
         * @tc.number:CustomHapticJsTest009
         */
        it("CustomHapticJsTest009", 0, async function (done) {
            function vibratePromise() {
                return new Promise((resolve, reject) => {
                    vibrator.startVibration({
                        type: "preset",
                        effectId: "haptic.clock.timer",
                        count: 1,
                    }, {
                        usage: "notification"
                    }, (error)=>{
                        if (error) {
                            reject(error);
                        } else {
                            resolve();
                        }
                    });
                });
            }

            function promise(fileFd) {
                return new Promise((resolve, reject) => {
                    vibrator.startVibration({
                        type: "file",
                        hapticFd: { fd: fileFd }
                    }, {
                        usage: "unknown"
                    }).then(() => {
                        resolve();
                    }, (error) => {
                        reject(error);
                    });
                });
            }

            let fileFd = await openFile(FILE_NAME);
            await promise(fileFd).then(() => {
                vibratePromise().then(() => {
                    expect(true).assertTrue();
                }, (error) => {
                    expect(false).assertTrue();
                });
            }, (error) => {
                expect(error.code).assertEqual(IS_NOT_SUPPORTED_CODE);
                expect(error.message).assertEqual(IS_NOT_SUPPORTED_MSG);
            });
            await closeFile(fileFd);
            done();
        })

        /*
         * @tc.name:CustomHapticJsTest
         * @tc.desc:Verification results of the incorrect parameters of the test interface.
         * @tc.number:CustomHapticJsTest010
         */
        it("CustomHapticJsTest010", 0, async function (done) {
            function vibratePromise() {
                return new Promise((resolve, reject) => {
                    vibrator.startVibration({
                        type: "time",
                        duration: 500,
                    }, {
                        usage: "unknown"
                    }, (error)=>{
                        if (error) {
                            reject(error);
                        } else {
                            resolve();
                        }
                    });
                });
            }

            function promise(fileFd) {
                return new Promise((resolve, reject) => {
                    vibrator.startVibration({
                        type: "file",
                        hapticFd: { fd: fileFd }
                    }, {
                        usage: "alarm"
                    }).then(() => {
                        resolve();
                    }, (error) => {
                        reject(error);
                    });
                });
            }

            let fileFd = await openFile(FILE_NAME);
            await promise(fileFd).then(() => {
                vibratePromise().then(() => {
                    expect(false).assertTrue();
                }, (error) => {
                    expect(true).assertTrue();
                });
            }, (error) => {
                expect(error.code).assertEqual(IS_NOT_SUPPORTED_CODE);
                expect(error.message).assertEqual(IS_NOT_SUPPORTED_MSG);
            });
            await closeFile(fileFd);
            done();
        })

        /*
         * @tc.name:CustomHapticJsTest
         * @tc.desc:Verification results of the incorrect parameters of the test interface.
         * @tc.number:CustomHapticJsTest011
         */
        it("CustomHapticJsTest011", 0, async function (done) {
            function vibratePromise(fileFd) {
                return new Promise((resolve, reject) => {
                    vibrator.startVibration({
                        type: "file",
                        hapticFd: { fd: fileFd }
                    }, {
                        usage: "notification"
                    }).then(() => {
                        resolve();
                    }, (error) => {
                        reject(error);
                    });
                });
            }

            function promise() {
                return new Promise((resolve, reject) => {
                    vibrator.startVibration({
                        type: "preset",
                        effectId: "haptic.clock.timer",
                        count: 1,
                    }, {
                        usage: "unknown"
                    }, (error)=>{
                        if (error) {
                            reject(error);
                        } else {
                            resolve();
                        }
                    });
                });
            }

            let fileFd = await openFile(FILE_NAME);
            await promise().then(() => {
                vibratePromise(fileFd).then(() => {
                    expect(true).assertTrue();
                }, (error) => {
                    expect(error.code).assertEqual(IS_NOT_SUPPORTED_CODE);
                    expect(error.msg).assertEqual(IS_NOT_SUPPORTED_MSG);
                });
            }, (error) => {
                expect(false).assertTrue();
            });
            await closeFile(fileFd);
        })

        /*
         * @tc.name:CustomHapticJsTest
         * @tc.desc:Verification results of the incorrect parameters of the test interface.
         * @tc.number:CustomHapticJsTest012
         */
        it("CustomHapticJsTest012", 0, async function (done) {
            function vibratePromise(fileFd) {
                return new Promise((resolve, reject) => {
                    vibrator.startVibration({
                        type: "file",
                        hapticFd: { fd: fileFd }
                    }, {
                        usage: "unknown"
                    }).then(() => {
                        resolve();
                    }, (error) => {
                        reject(error);
                    });
                });
            }

            function promise() {
                return new Promise((resolve, reject) => {
                    vibrator.startVibration({
                        type: "time",
                        duration: 500,
                    }, {
                        usage: "alarm"
                    }, (error)=>{
                        if (error) {
                            reject(error);
                        } else {
                            resolve();
                        }
                    });
                });
            }

            let fileFd = await openFile(FILE_NAME);
            await promise().then(() => {
                vibratePromise(fileFd).then(() => {
                    expect(false).assertTrue();
                }, (error) => {
                    expect(true).assertTrue();
                });
            }, (error) => {
                expect(false).assertTrue();
            });
            await closeFile(fileFd);
            done();
        })
    })}
