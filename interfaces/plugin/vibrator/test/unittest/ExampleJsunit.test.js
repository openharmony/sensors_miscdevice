/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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
import vibrator from '@ohos.vibrator'
import systemVibrator from '@system.vibrator'

import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'

describe("VibratorJsTest", function () {
    beforeAll(function() {
        /*
         * @tc.setup: setup invoked before all testcases
         */
         console.info('beforeAll called')
    })
    
    afterAll(function() {
        /*
         * @tc.teardown: teardown invoked after all testcases
         */
         console.info('afterAll called')
    })
    
    beforeEach(function() {
        /*
         * @tc.setup: setup invoked before each testcases
         */
         console.info('beforeEach called')
    })
    
    afterEach(function() {
        /*
         * @tc.teardown: teardown invoked after each testcases
         */
        vibrator.stop("preset");
        vibrator.stop("time");
        console.info('afterEach called')
    })

    const OPERATION_FAIL_CODE = 14600101;
    const PERMISSION_ERROR_CODE = 201;
    const PARAMETER_ERROR_CODE = 401;
    
    const OPERATION_FAIL_MSG = 'Device operation failed.'
    const PERMISSION_ERROR_MSG = 'Permission denied.'
    const PARAMETER_ERROR_MSG = 'The parameter invalid.'

    /*
     * @tc.name:VibratorJsTest001
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest001", 0, async function (done) {
        function vibrateCallback(error) {
            if (error) {
                console.info('VibratorJsTest001  vibrator error');
                expect(false).assertTrue();
            } else {
                console.info('VibratorJsTest001  vibrator success');
                expect(true).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        vibrator.vibrate(10, vibrateCallback);
    })

    /*
     * @tc.name:VibratorJsTest002
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest002", 0, async function (done) {
        function vibrateCallback(error) {
            if (error) {
                console.info('VibratorJsTest002  vibrator success');
                expect(true).assertTrue();
            } else {
                console.info('VibratorJsTest002  vibrator error');
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        vibrator.vibrate(-1, vibrateCallback);
    })

    /*
     * @tc.name:VibratorJsTest003
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest003", 0, async function (done) {
        function vibrateCallback(error) {
            if (error) {
                console.info('VibratorJsTest003  vibrator success');
                expect(true).assertTrue();
            } else {
                console.info('VibratorJsTest003  vibrator error');
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        vibrator.vibrate(1800000 + 1, vibrateCallback);
    })

    /*
     * @tc.name:VibratorJsTest004
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest004", 0, async function (done) {
        function vibrateCallback(error) {
            if (error) {
                console.info('VibratorJsTest004  vibrator error');
                expect(false).assertTrue();
            } else {
                console.info('VibratorJsTest004  vibrator success');
                expect(true).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        vibrator.vibrate(1800000, vibrateCallback);
    })

    /*
     * @tc.name:VibratorJsTest005
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest005", 0, async function (done) {
        function vibrateCallback(error) {
            if (error) {
                console.info('VibratorJsTest005  vibrator success');
                expect(true).assertTrue();
            } else {
                console.info('VibratorJsTest005  vibrator error');
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        vibrator.vibrate("", vibrateCallback);
    })

    /*
     * @tc.name:VibratorJsTest006
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest006", 0, async function (done) {
        function vibrateCallback(error) {
            if (error) {
                console.info('VibratorJsTest006  vibrator success');
                expect(true).assertTrue();
            } else {
                console.info('VibratorJsTest006  vibrator error');
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        vibrator.vibrate("xxx", vibrateCallback);
    })

    /*
     * @tc.name:VibratorJsTest007
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest007", 0, async function (done) {
        function vibrateCallback(error) {
            if (error) {
                console.info('VibratorJsTest007  vibrator error');
                expect(false).assertTrue();
            } else {
                console.info('VibratorJsTest007  vibrator success');
                expect(true).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        vibrator.vibrate("haptic.clock.timer", vibrateCallback);
    })

    /*
     * @tc.name:VibratorJsTest008
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest008", 0, async function (done) {
        function stopPromise() {
            return new Promise((resolve, reject) => {
                vibrator.stop("preset", (error)=>{
                    if (error) {
                        console.info('VibratorJsTest008 stop error');
                        expect(false).assertTrue();
                        setTimeout(()=>{
                            reject();
                        }, 500);
                    } else {
                        console.info('VibratorJsTest008 stop success');
                        expect(true).assertTrue();
                        setTimeout(()=>{
                            resolve();
                        }, 500);
                    }
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.vibrate("haptic.clock.timer", (error)=>{
                if (error) {
                    console.info('VibratorJsTest008  vibrate error');
                    expect(false).assertTrue();
                    setTimeout(()=>{
                        reject();
                    }, 500);
                } else {
                    console.info('VibratorJsTest008  vibrate success');
                    expect(true).assertTrue();
                    setTimeout(()=>{
                        resolve();
                    }, 500);
                }
            });
        })

        await promise.then(() =>{
            return stopPromise();
        }, ()=>{
            console.info("VibratorJsTest008 reject");
        })
        done();
    })

    /*
     * @tc.name:VibratorJsTest009
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest009", 0, async function (done) {
        console.info('----------------------SensorJsTest001---------------------------');
        function stopPromise() {
            return new Promise((resolve, reject) => {
                vibrator.stop("time", (error)=>{
                    if (error) {
                        console.info('VibratorJsTest009 stop error');
                        expect(false).assertTrue();
                        setTimeout(()=>{
                            reject();
                        }, 500);
                    } else {
                        console.info('VibratorJsTest009 stop success');
                        expect(true).assertTrue();
                        setTimeout(()=>{
                            resolve();
                        }, 500);
                    }
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.vibrate(180000, (error)=>{
                if (error) {
                    console.info('VibratorJsTest009  vibrate error');
                    expect(false).assertTrue();
                    setTimeout(()=>{
                        reject();
                    }, 500);
                } else {
                    console.info('VibratorJsTest009  vibrate success');
                    expect(true).assertTrue();
                    setTimeout(()=>{
                        resolve();
                    }, 500);
                }
            });
        })

        await promise.then(() =>{
            return stopPromise();
        }, ()=>{
            console.info("VibratorJsTest009 reject");
        })
        done();
    })

    /*
     * @tc.name:VibratorJsTest010
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest010", 0, async function (done) {
        try {
            function vibrateCallback(error) {
                if (error) {
                    console.info('VibratorJsTest010  stop success');
                    expect(true).assertTrue();
                } else {
                    console.info('VibratorJsTest010  stop off');
                    expect(false).assertTrue();
                }
                setTimeout(()=>{
                    done();
                }, 500);
            }
            vibrator.stop("", vibrateCallback);
        } catch (error) {
            console.info(error);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }   
    })

    /*
     * @tc.name:VibratorJsTest011
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest011", 0, async function (done) {
        vibrator.vibrate(1000).then(() => {
            console.log("VibratorJsTest011  vibrate success");
            expect(true).assertTrue();
            setTimeout(()=>{
                done();
            }, 500);
        }, (error)=>{
            expect(false).assertTrue();
            console.log("VibratorJsTest011  vibrate error");
            setTimeout(()=>{
                done();
            }, 500);
        });
    })

    /*
     * @tc.name:VibratorJsTest012
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest012", 0, async function (done) {
        vibrator.vibrate(-1).then(() => {
            console.log("VibratorJsTest012  vibrate error");
            expect(false).assertTrue();
            setTimeout(()=>{
                done();
            }, 500);
        }, (error)=>{
            expect(true).assertTrue();
            console.log("VibratorJsTest012  vibrate success");
            setTimeout(()=>{
                done();
            }, 500);
        });
    })

    /*
     * @tc.name:VibratorJsTest013
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest013", 0, async function (done) {
        vibrator.vibrate(1800000 + 1).then(() => {
            console.log("VibratorJsTest013  vibrate error");
            expect(false).assertTrue();
            setTimeout(()=>{
                done();
            }, 500);
        }, (error)=>{
            expect(true).assertTrue();
            console.log("VibratorJsTest013  vibrate success");
            setTimeout(()=>{
                done();
            }, 500);
        });
    })

    /*
     * @tc.name:VibratorJsTest014
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest014", 0, async function (done) {
        vibrator.vibrate("haptic.clock.timer").then(() => {
            console.log("VibratorJsTest014  vibrate success");
            expect(true).assertTrue();
            setTimeout(()=>{
                done();
            }, 500);
        }, (error)=>{
            expect(false).assertTrue();
            console.log("VibratorJsTest014  vibrate error");
            setTimeout(()=>{
                done();
            }, 500);
        });
    })

    /*
     * @tc.name:VibratorJsTest015
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest015", 0, async function (done) {
        function stopPromise() {
            return new Promise((resolve, reject) => {
                vibrator.stop("preset").then(() => {
                    console.log("VibratorJsTest015  off success");
                    expect(true).assertTrue();
                    setTimeout(()=>{
                        resolve();
                    }, 500);
                }, (error)=>{
                    expect(false).assertTrue();
                    console.log("VibratorJsTest015  off error");
                    setTimeout(()=>{
                        reject();
                    }, 500);
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.vibrate("haptic.clock.timer").then(() => {
                console.log("VibratorJsTest015  vibrate success");
                expect(true).assertTrue();
                setTimeout(()=>{
                    resolve();
                }, 500);
            }, (error)=>{
                expect(false).assertTrue();
                console.log("VibratorJsTest015  vibrate error");
                setTimeout(()=>{
                    reject();
                }, 500);
            });
        })

        await promise.then(() =>{
            return stopPromise();
        }, ()=>{
            console.info("VibratorJsTest009 reject");
        })
        done();
    })

    /*
     * @tc.name:VibratorJsTest016
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest016", 0, async function (done) {
        vibrator.vibrate("").then(() => {
            console.log("VibratorJsTest016  vibrate error");
            expect(false).assertTrue();
            setTimeout(()=>{
                done();
            }, 500);
        }, (error)=>{
            expect(true).assertTrue();
            console.log("VibratorJsTest016  vibrate success");
            setTimeout(()=>{
                done();
            }, 500);
        });
    })

    /*
     * @tc.name:VibratorJsTest017
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest017", 0, async function (done) {
        try {
            vibrator.stop("").then(() => {
                console.log("VibratorJsTest017  stop error");
                expect(false).assertTrue();
                setTimeout(()=>{
                    done();
                }, 500);
            }, (error)=>{
                expect(true).assertTrue();
                console.log("VibratorJsTest017  stop success");
                setTimeout(()=>{
                    done();
                }, 500);
            });
        } catch (error) {
            console.info(error);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
     * @tc.name:VibratorJsTest018
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest018", 0, async function (done) {
        function stopPromise() {
            return new Promise((resolve, reject) => {
                vibrator.stop("time").then(() => {
                    console.log("VibratorJsTest018  stop success");
                    expect(true).assertTrue();
                    setTimeout(()=>{
                        resolve();
                    }, 500);
                }, (error)=>{
                    expect(false).assertTrue();
                    console.log("VibratorJsTest018  stop error");
                    setTimeout(()=>{
                        reject();
                    }, 500);
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.vibrate(180000).then(() => {
                console.log("VibratorJsTest018  vibrate success");
                expect(true).assertTrue();
                setTimeout(()=>{
                    resolve();
                }, 500);
            }, (error)=>{
                expect(false).assertTrue();
                console.log("VibratorJsTest018  vibrate error");
                setTimeout(()=>{
                    reject();
                }, 500);
            });
        })

        await promise.then(() =>{
            return stopPromise();
        }, ()=>{
            console.info("VibratorJsTest018 reject");
        })
        done();
    })

    /*
     * @tc.name:VibratorJsTest019
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest019", 0, async function (done) {
        systemVibrator.vibrate({
            mode: 'short',
            success: function() {
              expect(true).assertTrue();
              console.log('vibrate is successful');
              done();
            },
            fail: function(data, code) {
              expect(false).assertTrue();
              console.log('vibrate is failed, data: ' + data + "code: " + code);
              done();
            },
            complete: function() {
              console.log('vibrate is completed');
              done();
            }
          });
    })

    /*
     * @tc.name:VibratorJsTest020
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest020", 0, async function (done) {
        systemVibrator.vibrate({
            mode: 'long',
            success: function() {
              expect(true).assertTrue();
              console.log('vibrate is successful');
              done();
            },
            fail: function(data, code) {
              expect(false).assertTrue();
              console.log('vibrate is failed, data: ' + data + "code: " + code);
              done();
            },
            complete: function() {
              console.log('vibrate is completed');
              done();
            }
          });
    })

    /*
     * @tc.name:VibratorJsTest021
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest021", 0, async function (done) {
        systemVibrator.vibrate({
            success: function() {
              expect(true).assertTrue();
              console.log('vibrate is successful');
              done();
            },
            fail: function(data, code) {
              expect(false).assertTrue();
              console.log('vibrate is failed, data: ' + data + "code: " + code);
              done();
            },
            complete: function() {
              console.log('vibrate is completed');
              done();
            }
          });
    })

    /*
     * @tc.name:VibratorJsTest022
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("VibratorJsTest022", 0, async function (done) {
        systemVibrator.vibrate({
            success: function() {
              expect(true).assertTrue();
              console.log('vibrate is successful');
              done();
            },
          });
    })

    /*
     * @tc.name:VibrateTest001
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest001", 0, async function (done) {
        vibrator.startVibration({
            type: "time",
            duration: 1000
        }, {
            usage: "unknown"
        }, (error)=>{
            if (error) {
                console.info('VibrateTest001 startVibration error');
                expect(false).assertTrue();
            } else {
                console.info('VibrateTest001 startVibration success');
                expect(true).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        });
    })

    /*
     * @tc.name:VibrateTest002
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest002", 0, async function (done) {
        try {
            vibrator.startVibration({
                type: "",
                duration: 1000
            }, {
                usage: "unknown"
            }, (error)=>{
                if (error) {
                    expect(true).assertTrue();
                } else {
                    expect(false).assertTrue();
                }
                setTimeout(()=>{
                    done();
                }, 500);
            });
        } catch (error) {
            console.info(error);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
     * @tc.name:VibrateTest003
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest003", 0, async function (done) {
        vibrator.startVibration({
            type: "preset",
            effectId: "haptic.clock.timer",
            count: 1,
        }, {
            usage: "unknown"
        }, (error)=>{
            if (error) {
                console.info('VibrateTest003 vibrator error');
                expect(false).assertTrue();
            } else {
                console.info('VibrateTest003 vibrator success');
                expect(true).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        });
    })

    /*
     * @tc.name:VibrateTest004
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest004", 0, async function (done) {
        try {
            vibrator.startVibration({
                type: "preset",
                effectId: "",
                count: 3,
            }, {
                usage: "unknown"
            }, (error)=>{
                if (error) {
                    expect(true).assertTrue();
                } else {
                    expect(false).assertTrue();
                }
                setTimeout(()=>{
                    done();
                }, 500);
            });
        } catch (error) {
            console.info(error);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
     * @tc.name:VibrateTest005
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest005", 0, async function (done) {
        try {
            vibrator.startVibration({
                type: "preset",
                effectId: "haptic.clock.timer",
                count: 3,
            }, {
                usage: ""
            }, (error)=>{
                expect(false).assertTrue();
                setTimeout(()=>{
                    done();
                }, 500);
            });
        } catch (error) {
            console.info(error);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
     * @tc.name:VibrateTest006
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest006", 0, async function (done) {
        try {
            vibrator.startVibration(null, null);
        } catch (error) {
            console.info(error);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
     * @tc.name:VibrateTest007
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest007", 0, async function (done) {
        await vibrator.startVibration({
            type: "time",
            duration: 1000,
        }, {
            usage: "unknown"
        }).then(()=>{
            expect(true).assertTrue();
        }).catch((error)=>{
            expect(false).assertTrue();
        });
        done();
    })

    /*
     * @tc.name:VibrateTest008
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest008", 0, async function (done) {
        try {
            await vibrator.startVibration({
                type: "",
                duration: 1000
            }, {
                usage: "unknown"
            }).then(()=>{
                expect(false).assertTrue();
            }).catch((error)=>{
                expect(true).assertTrue();
            });
            done();
        } catch (error) {
            console.info(error);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
     * @tc.name:VibrateTest009
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest009", 0, async function (done) {
        await vibrator.startVibration({
            type: "preset",
            effectId: "haptic.clock.timer",
            count: 1,
        }, {
            usage: "unknown"
        }).then(()=>{
            expect(true).assertTrue();
        }).catch((error)=>{
            expect(false).assertTrue();
        });
        done();
    })

    /*
     * @tc.name:VibrateTest010
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest010", 0, async function (done) {
        try {
            vibrator.startVibration({
                type: "preset",
                effectId: "",
                count: 3,
            }, {
                usage: "unknown"
            }).then(()=>{
                expect(false).assertTrue();
                done();
            }).catch((error)=>{
                expect(true).assertTrue();
                done();
            });
        } catch (error) {
            console.info(error);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
     * @tc.name:VibrateTest011
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest011", 0, async function (done) {
        try {
            vibrator.startVibration({
                type: "preset",
                effectId: "haptic.clock.timer",
                count: 3,
            }, {
                usage: ""
            }).then(()=>{
                expect(false).assertTrue();
                done();
            }).catch((error)=>{
                expect(true).assertTrue();
                done();
            });
        } catch (error) {
            console.info(error);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
     * @tc.name:VibrateTest012
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest012", 0, async function (done) {
        try {
            vibrator.startVibration({
                type: 1,
                count: 3,
            }, {
                usage: ""
            })
        } catch (error) {
            console.info(error);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
     * @tc.name:VibrateTest013
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest013", 0, async function (done) {
        function vibratePromise() {
            return new Promise((resolve, reject) => {
                vibrator.startVibration({
                    type: "time",
                    duration: 100
                }, {
                    usage: "unknown"
                }, (error)=>{
                    if (error) {
                        expect(false).assertTrue();
                    } else {
                        expect(true).assertTrue();
                    }
                    setTimeout(()=>{
                        done();
                    }, 500);
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.startVibration({
                type: "time",
                duration: 100
            }, {
                usage: "unknown"
            }, (error)=>{
                if (error) {
                    expect(false).assertTrue();
                    reject();
                } else {
                    expect(true).assertTrue();
                    resolve();
                }
            });
        })

        await promise.then(() =>{
            return vibratePromise();
        }, ()=>{
            console.info("StartVibrationTest013 reject");
        })
        done();
    })

    /*
     * @tc.name:VibrateTest014
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest014", 0, async function (done) {
        function vibratePromise() {
            return new Promise((resolve, reject) => {
                vibrator.startVibration({
                    type: "preset",
                    effectId: "haptic.clock.timer",
                    count: 1,
                }, {
                    usage: "unknown"
                }, (error)=>{
                    if (error) {
                        expect(error.code).assertEqual(OPERATION_FAIL_CODE);
                        expect(error.message).assertEqual(OPERATION_FAIL_MSG);
                    } else {
                        expect(false).assertTrue();
                    }
                    setTimeout(()=>{
                        done();
                    }, 500);
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.startVibration({
                type: "time",
                duration: 100
            }, {
                usage: "alarm"
            }, (error)=>{
                if (error) {
                    expect(false).assertTrue();
                    reject();
                } else {
                    expect(true).assertTrue();
                    resolve();
                }
            });
        })

        await promise.then(() =>{
            return vibratePromise();
        }, ()=>{
            console.info("VibrateTest014 reject");
        })
        done();
    })

    /*
     * @tc.name:VibrateTest015
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest015", 0, async function (done) {
        function vibratePromise() {
            return new Promise((resolve, reject) => {
                vibrator.startVibration({
                    type: "preset",
                    effectId: "haptic.clock.timer",
                    count: 3,
                }, {
                    usage: "unknown",
                }, (error)=>{
                    if (error) {
                        expect(false).assertTrue();
                    } else {
                        expect(true).assertTrue();
                    }
                    setTimeout(()=>{
                        done();
                    }, 500);
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.startVibration({
                type: "time",
                duration: 10000
            }, {
                usage: "alarm"
            }, (error)=>{
                if (error) {
                    expect(false).assertTrue();
                    reject();
                } else {
                    expect(true).assertTrue();
                    resolve();
                }
            });
        })
        await promise.then(() =>{
            return vibratePromise();
        }, ()=>{
            console.info("VibrateTest015 reject");
        })
        done();
    })

    /*
     * @tc.name:VibrateTest016
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest016", 0, async function (done) {
        function vibratePromise() {
            return new Promise((resolve, reject) => {
                vibrator.startVibration({
                    type: "preset",
                    effectId: "haptic.clock.timer",
                    count: 1,
                }, {
                    usage: "unknown",
                }, (error)=>{
                    if (error) {
                        expect(error.code).assertEqual(OPERATION_FAIL_CODE);
                        expect(error.message).assertEqual(OPERATION_FAIL_MSG);
                    } else {
                        expect(false).assertTrue();
                    }
                    setTimeout(()=>{
                        done();
                    }, 500);
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.startVibration({
                type: "preset",
                effectId: "haptic.clock.timer",
                count: 3,
            }, {
                usage: "unknown",
            }, (error)=>{
                if (error) {
                    expect(false).assertTrue();
                    reject();
                } else {
                    expect(true).assertTrue();
                    resolve();
                }
            });
        })

        await promise.then(() =>{
            return vibratePromise();
        }, ()=>{
            console.info("VibrateTest016 reject");
        })
        done();
    })

    /*
     * @tc.name:VibrateTest017
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest017", 0, async function (done) {
        function vibratePromise() {
            return new Promise((resolve, reject) => {
                vibrator.startVibration({
                    type: "time",
                    duration: 3000,
                }, {
                    usage: "alarm"
                }, (error)=>{
                    if (error) {
                        expect(error.code).assertEqual(OPERATION_FAIL_CODE);
                        expect(error.message).assertEqual(OPERATION_FAIL_MSG);
                    } else {
                        expect(false).assertTrue();
                    }
                    setTimeout(()=>{
                        done();
                    }, 500);
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.startVibration({
                type: "preset",
                effectId: "haptic.clock.timer",
                count: 3,
            }, {
                usage: "unknown"
            }, (error)=>{
                if (error) {
                    expect(false).assertTrue();
                    reject();
                } else {
                    expect(true).assertTrue();
                    resolve();
                }
            });
        })

        await promise.then(() =>{
            return vibratePromise();
        }, ()=>{
            console.info("VibrateTest017 reject");
        })
        done();
    })

    /*
     * @tc.name:VibrateTest018
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest018", 0, async function (done) {
        function vibratePromise() {
            return new Promise((resolve, reject) => {
                vibrator.startVibration({
                    type: "time",
                    duration: 3000,
                }, {
                    usage: "alarm"
                }, (error)=>{
                    if (error) {
                        expect(false).assertTrue();
                    } else {
                        expect(true).assertTrue();
                    }
                    setTimeout(()=>{
                        done();
                    }, 500);
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.startVibration({
                type: "preset",
                effectId: "haptic.clock.timer",
                count: 1,
            }, {
                usage: "unknown"
            }, (error)=>{
                if (error) {
                    expect(false).assertTrue();
                    reject();
                } else {
                    expect(true).assertTrue();
                    resolve();
                }
            });
        })

        await promise.then(() =>{
            return vibratePromise();
        }, ()=>{
            console.info("VibrateTest018 reject");
        })
        done();
    })

    /*
     * @tc.name:VibrateTest019
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest019", 0, async function (done) {
        function vibratePromise() {
            return new Promise((resolve, reject) => {
                vibrator.startVibration({
                    type: "preset",
                    effectId: "haptic.clock.timer",
                    count: 3,
                }, {
                    usage: "unknown"
                }, (error)=>{
                    if (error) {
                        expect(false).assertTrue();
                    } else {
                        expect(true).assertTrue();
                    }
                    setTimeout(()=>{
                        done();
                    }, 500);
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.startVibration({
                type: "preset",
                effectId: "haptic.clock.timer",
                count: 3,
            }, {
                usage: "unknown"
            }, (error)=>{
                if (error) {
                    expect(false).assertTrue();
                    reject();
                } else {
                    expect(true).assertTrue();
                    resolve();
                }
            });
        })

        await promise.then(() =>{
            return vibratePromise();
        }, ()=>{
            console.info("VibrateTest019 reject");
        })
        done();
    })

    /*
     * @tc.name:VibrateTest020
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest020", 0, async function (done) {
        function vibratePromise() {
            return new Promise((resolve, reject) => {
                vibrator.startVibration({
                    type: "preset",
                    effectId: "haptic.clock.timer",
                    count: 1,
                }, {
                    usage: "ring"
                }, (error)=>{
                    if (error) {
                        expect(false).assertTrue();
                    } else {
                        expect(true).assertTrue();
                    }
                    setTimeout(()=>{
                        done();
                    }, 500);
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.startVibration({
                type: "preset",
                effectId: "haptic.clock.timer",
                count: 1,
            }, {
                usage: "notification"
            }, (error)=>{
                if (error) {
                    expect(false).assertTrue();
                    reject();
                } else {
                    expect(true).assertTrue();
                    resolve();
                }
            });
        })

        await promise.then(() =>{
            return vibratePromise();
        }, ()=>{
            console.info("VibrateTest020 reject");
        })
        done();
    })

    /*
     * @tc.name:VibrateTest021
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I53SGE
     */
    it("VibrateTest021", 0, async function (done) {
        function vibratePromise() {
            return new Promise((resolve, reject) => {
                vibrator.startVibration({
                    type: "preset",
                    effectId: "haptic.clock.timer",
                    count: 1,
                }, {
                    usage: "unknown"
                }, (error)=>{
                    if (error) {
                        expect(error.code).assertEqual(OPERATION_FAIL_CODE);
                        expect(error.message).assertEqual(OPERATION_FAIL_MSG);
                    } else {
                        expect(false).assertTrue();
                    }
                    setTimeout(()=>{
                        done();
                    }, 500);
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.startVibration({
                type: "preset",
                effectId: "haptic.clock.timer",
                count: 1,
            }, {
                usage: "notification"
            }, (error)=>{
                if (error) {
                    expect(false).assertTrue();
                    reject();
                } else {
                    expect(true).assertTrue();
                    resolve();
                }
            });
        })

        await promise.then(() =>{
            return vibratePromise();
        }, ()=>{
            console.info("VibrateTest020 reject");
        })
        done();
    })

     /*
     * @tc.name:VibratorJsTest022
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I5SWJI
     */
     it("VibrateTest022", 0, async function (done) {
        function vibrateCallback(error) {
            if (error) {
                console.info('VibrateTest022  stop fail');
                expect(false).assertTrue();
            } else {
                console.info('VibrateTest022  stop off');
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        try {
            vibrator.stopVibration("", vibrateCallback);
        } catch (error) {
            console.info(error);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
     * @tc.name:VibrateTest023
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I5SWJI
     */
    it("VibrateTest023", 0, async function (done) {
        function stopPromise() {
            return new Promise((resolve, reject) => {
                vibrator.stopVibration("preset").then(() => {
                    console.log("VibrateTest023  off success");
                    expect(true).assertTrue();
                    setTimeout(()=>{
                        resolve();
                    }, 500);
                }, (error)=>{
                    expect(false).assertTrue();
                    console.log("VibrateTest023  off error");
                    setTimeout(()=>{
                        reject();
                    }, 500);
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.startVibration("haptic.clock.timer").then(() => {
                console.log("VibrateTest023  vibrate success");
                expect(true).assertTrue();
                setTimeout(()=>{
                    resolve();
                }, 500);
            }, (error)=>{
                expect(false).assertTrue();
                console.log("VibrateTest023  vibrate error");
                setTimeout(()=>{
                    reject();
                }, 500);
            });
        })

        await promise.then(() =>{
            return stopPromise();
        }, ()=>{
            console.info("VibrateTest023 reject");
        })
        done();
    })

    /*
     * @tc.name:VibrateTest024
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I5SWJI
     */
    it("VibrateTest024", 0, async function (done) {
        try {
            vibrator.stopVibration("").then(() => {
                console.log("VibrateTest024  stop error");
                expect(false).assertTrue();
                setTimeout(()=>{
                    done();
                }, 500);
            }, (error)=>{
                expect(false).assertTrue();
                console.log("VibrateTest024 stop error");
                setTimeout(()=>{
                    done();
                }, 500);
            });
        } catch (error) {
            console.info(error);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
     * @tc.name:VibrateTest025
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: I5SWJI
     */
    it("VibrateTest025", 0, async function (done) {
        function stopPromise() {
            return new Promise((resolve, reject) => {
                vibrator.stopVibration("time").then(() => {
                    console.log("VibrateTest025 stop success");
                    expect(true).assertTrue();
                    setTimeout(()=>{
                        resolve();
                    }, 500);
                }, (error)=>{
                    expect(false).assertTrue();
                    console.log("VibrateTest025 stop error");
                    setTimeout(()=>{
                        reject();
                    }, 500);
                });
            })
        }

        let promise = new Promise((resolve, reject) => {
            vibrator.vibrate(180000).then(() => {
                console.log("VibrateTest025 vibrate success");
                expect(true).assertTrue();
                setTimeout(()=>{
                    resolve();
                }, 500);
            }, (error)=>{
                expect(false).assertTrue();
                console.log("VibrateTest025  vibrate error");
                setTimeout(()=>{
                    reject();
                }, 500);
            });
        })

        await promise.then(() =>{
            return stopPromise();
        }, ()=>{
            console.info("VibrateTest025 reject");
        })
        done();
    })
})
