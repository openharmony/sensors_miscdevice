/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <string>
#include <thread>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "parameters.h"
#include "parcel.h"
#include "token_setproc.h"

#include "sensors_errors.h"
#include "vibrator_agent.h"
#include "vibrator_infos.h"

#undef LOG_TAG
#define LOG_TAG "VibratorAgentTest"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
using namespace Security::AccessToken;
using Security::AccessToken::AccessTokenID;

namespace {
constexpr int32_t TIME_WAIT_FOR_OP = 500;
constexpr int32_t TIME_WAIT_FOR_OP_EACH_CASE = 200;
constexpr int32_t INTENSITY_HIGH = 100;
constexpr int32_t INTENSITY_MEDIUM = 50;
constexpr int32_t INTENSITY_LOW = 20;
constexpr int32_t INTENSITY_INVALID = -1;

PermissionStateFull g_infoManagerTestState = {
    .grantFlags = {1},
    .grantStatus = {PermissionState::PERMISSION_GRANTED},
    .isGeneral = true,
    .permissionName = "ohos.permission.VIBRATE",
    .resDeviceID = {"local"}
};

HapPolicyParams g_infoManagerTestPolicyPrams = {
    .apl = APL_NORMAL,
    .domain = "test.domain",
    .permList = {},
    .permStateList = {g_infoManagerTestState}
};

HapInfoParams g_infoManagerTestInfoParms = {
    .bundleName = "vibratoragent_test",
    .userID = 1,
    .instIndex = 0,
    .appIDDesc = "vibratorAgentTest"
};

void TestCallBack(VibratorStatusEvent *deviceInfo)
{
    return;
}

constexpr VibratorUser testUser = {
    .callback = TestCallBack,
};
} // namespace

class VibratorAgentTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

private:
    static AccessTokenID tokenID_;
};

struct FileDescriptor {
    explicit FileDescriptor(const std::string &path)
    {
        fd = open(path.c_str(), O_RDONLY);
    }
    ~FileDescriptor()
    {
        close(fd);
    }
    int32_t fd;
};

AccessTokenID VibratorAgentTest::tokenID_ = 0;

void VibratorAgentTest::SetUpTestCase()
{
    AccessTokenIDEx tokenIdEx = {0};
    tokenIdEx = AccessTokenKit::AllocHapToken(g_infoManagerTestInfoParms, g_infoManagerTestPolicyPrams);
    tokenID_ = tokenIdEx.tokenIdExStruct.tokenID;
    ASSERT_NE(0, tokenID_);
    ASSERT_EQ(0, SetSelfTokenID(tokenID_));
}

void VibratorAgentTest::TearDownTestCase()
{
    int32_t ret = AccessTokenKit::DeleteToken(tokenID_);
    if (tokenID_ != 0) {
        ASSERT_EQ(RET_SUCCESS, ret);
    }
}

void VibratorAgentTest::SetUp()
{}

void VibratorAgentTest::TearDown()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_EACH_CASE));
}

bool IsSupportVibratorEffect(const char* effectId)
{
    bool state { false };
    IsSupportEffect(effectId, &state);
    return state;
}

HWTEST_F(VibratorAgentTest, StartVibratorTest_001, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorTest_001 in");
    bool isSupport = IsSupportVibratorEffect(VIBRATOR_TYPE_CLOCK_TIMER);
    if (isSupport) {
        int32_t ret = StartVibrator(VIBRATOR_TYPE_CLOCK_TIMER);
        ASSERT_EQ(ret, 0);
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, StartVibratorTest_002, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorTest_002 in");
    int32_t ret = StartVibrator("");
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StartVibratorTest_003, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorTest_003 in");
    int32_t ret = StartVibrator(nullptr);
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StartVibratorOnceTest_001, TestSize.Level0)
{
    MISC_HILOGI("StartVibratorOnceTest_001 in");
    int32_t ret = StartVibratorOnce(300);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, StartVibratorOnceTest_002, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorOnceTest_002 in");
    int32_t ret = StartVibratorOnce(0);
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StartVibratorOnceTest_003, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorOnceTest_003 in");
    int32_t ret = StartVibratorOnce(1800000);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, StartVibratorOnceTest_004, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorOnceTest_004 in");
    int32_t ret = StartVibratorOnce(1800001);
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StopVibratorTest_001, TestSize.Level1)
{
    MISC_HILOGI("StopVibratorTest_001 in");
    int32_t ret = StopVibrator("time");
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, StopVibratorTest_002, TestSize.Level1)
{
    MISC_HILOGI("StopVibratorTest_002 in");
    int32_t ret = StopVibrator("preset");
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StopVibratorTest_003, TestSize.Level1)
{
    MISC_HILOGI("StopVibratorTest_003 in");
    int32_t ret = StopVibrator("");
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StopVibratorTest_004, TestSize.Level1)
{
    MISC_HILOGI("StopVibratorTest_004 in");
    int32_t ret = StopVibrator(nullptr);
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StopVibratorTest_005, TestSize.Level1)
{
    MISC_HILOGI("StopVibratorTest_005 in");
    int32_t ret = StartVibratorOnce(300);
    ASSERT_EQ(ret, 0);
    ret = StopVibrator("time");
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, SetLoopCount_001, TestSize.Level1)
{
    MISC_HILOGI("SetLoopCount_001 in");
    bool ret = SetLoopCount(300);
    ASSERT_TRUE(ret);
}

HWTEST_F(VibratorAgentTest, SetLoopCount_002, TestSize.Level1)
{
    MISC_HILOGI("SetLoopCount_002 in");
    bool ret = SetLoopCount(-1);
    ASSERT_FALSE(ret);
}

HWTEST_F(VibratorAgentTest, SetLoopCount_003, TestSize.Level1)
{
    MISC_HILOGI("SetLoopCount_003 in");
    bool ret = SetLoopCount(0);
    ASSERT_FALSE(ret);
}

HWTEST_F(VibratorAgentTest, SetUsage_001, TestSize.Level1)
{
    MISC_HILOGI("SetUsage_001 in");
    bool ret = SetUsage(0);
    ASSERT_TRUE(ret);
}

HWTEST_F(VibratorAgentTest, SetUsage_002, TestSize.Level1)
{
    MISC_HILOGI("SetUsage_002 in");
    bool ret = SetUsage(-1);
    ASSERT_FALSE(ret);
}

HWTEST_F(VibratorAgentTest, SetUsage_003, TestSize.Level1)
{
    MISC_HILOGI("SetUsage_003 in");
    bool ret = SetUsage(USAGE_MAX);
    ASSERT_FALSE(ret);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_001, TestSize.Level0)
{
    MISC_HILOGI("PlayVibratorCustom_001 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_002, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_002 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_003, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_003 in");
    bool isSupport = (IsSupportVibratorCustom() && IsSupportVibratorEffect(VIBRATOR_TYPE_FAIL));
    if (isSupport) {
        bool flag = SetLoopCount(2);
        ASSERT_TRUE(flag);
        int32_t ret = StartVibrator(VIBRATOR_TYPE_FAIL);
        ASSERT_EQ(ret, 0);
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_004, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_004 in");
    bool isSupport = (IsSupportVibratorCustom() && IsSupportVibratorEffect(VIBRATOR_TYPE_FAIL));
    if (isSupport) {
        bool flag = SetUsage(USAGE_ALARM);
        ASSERT_TRUE(flag);
        int32_t ret = StartVibrator(VIBRATOR_TYPE_FAIL);
        ASSERT_EQ(ret, 0);
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_005, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_005 in");
    bool isSupport = (IsSupportVibratorCustom() && IsSupportVibratorEffect(VIBRATOR_TYPE_FAIL));
    if (isSupport) {
        bool flag = SetUsage(USAGE_UNKNOWN);
        ASSERT_TRUE(flag);
        int32_t ret = StartVibrator(VIBRATOR_TYPE_FAIL);
        ASSERT_EQ(ret, 0);
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_006, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_006 in");
    bool isSupport = (IsSupportVibratorCustom() && IsSupportVibratorEffect(VIBRATOR_TYPE_FAIL));
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            bool flag = SetUsage(USAGE_ALARM);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = StartVibrator(VIBRATOR_TYPE_FAIL);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_007, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_007 in");
    bool isSupport = (IsSupportVibratorCustom() && IsSupportVibratorEffect(VIBRATOR_TYPE_FAIL));
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            bool flag = SetUsage(USAGE_UNKNOWN);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = StartVibrator(VIBRATOR_TYPE_FAIL);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_008, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_008 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        bool flag = SetUsage(USAGE_ALARM);
        ASSERT_TRUE(flag);
        int32_t ret = StartVibratorOnce(500);
        ASSERT_EQ(ret, 0);
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_009, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_009 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        bool flag = SetUsage(USAGE_UNKNOWN);
        ASSERT_TRUE(flag);
        int32_t ret = StartVibratorOnce(500);
        ASSERT_EQ(ret, 0);
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_010, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_010 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            bool flag = SetUsage(USAGE_ALARM);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = StartVibratorOnce(500);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_011, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_011 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            bool flag = SetUsage(USAGE_UNKNOWN);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = StartVibratorOnce(500);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_012, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_012 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_128_event.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_013, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_013 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_type.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_014, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_014 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_startTime.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_015, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_015 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_duration.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_016, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_016 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_intensity.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_017, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_017 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_frequency.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_018, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_018 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_129_event.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_019, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_019 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_big_file_size.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_020, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_020 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_event_overlap_1.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_021, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_021 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_event_overlap_2.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}
HWTEST_F(VibratorAgentTest, PlayVibratorCustom_022, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_022 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/Jet_N2O.he");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustom_023, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_023 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/Racing_Start.he");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, SetParameters_001, TestSize.Level1)
{
    MISC_HILOGI("SetParameters_001 in");
    VibratorParameter parameter = {
        .intensity = -1,
        .frequency = -15
    };
    bool ret = SetParameters(parameter);
    ASSERT_FALSE(ret);
}

HWTEST_F(VibratorAgentTest, SetParameters_002, TestSize.Level1)
{
    MISC_HILOGI("SetParameters_002 in");
    VibratorParameter parameter = {
        .intensity = 70,
        .frequency = 150
    };
    bool ret = SetParameters(parameter);
    ASSERT_FALSE(ret);
}

HWTEST_F(VibratorAgentTest, SetParameters_003, TestSize.Level1)
{
    MISC_HILOGI("SetParameters_003 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            VibratorParameter parameter = {
                .intensity = 50,
                .frequency = -15
            };
            bool flag = SetParameters(parameter);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, SetParameters_004, TestSize.Level1)
{
    MISC_HILOGI("SetParameters_004 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            VibratorParameter parameter = {
                .intensity = 33,
                .frequency = 55
            };
            bool flag = SetParameters(parameter);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, Cancel_001, TestSize.Level1)
{
    MISC_HILOGI("Cancel_001 in");
    int32_t ret = Cancel();
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, Cancel_002, TestSize.Level1)
{
    MISC_HILOGI("Cancel_002 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = Cancel();
            ASSERT_EQ(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, Cancel_003, TestSize.Level1)
{
    MISC_HILOGI("Cancel_003 in");
    int32_t ret = StartVibratorOnce(500);
    ASSERT_EQ(ret, 0);
    ret = Cancel();
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, Cancel_004, TestSize.Level1)
{
    MISC_HILOGI("Cancel_004 in");
    if (IsSupportVibratorEffect(VIBRATOR_TYPE_FAIL)) {
        int32_t ret = StartVibrator(VIBRATOR_TYPE_FAIL);
        ASSERT_EQ(ret, 0);
        ret = Cancel();
        ASSERT_EQ(ret, 0);
    }
    ASSERT_TRUE(true);
}

HWTEST_F(VibratorAgentTest, IsSupportEffect_001, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffect_001 in");
    bool state { false };
    int32_t ret = IsSupportEffect(VIBRATOR_TYPE_CLOCK_TIMER, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibrator(VIBRATOR_TYPE_CLOCK_TIMER);
        ASSERT_EQ(ret, 0);
        Cancel();
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_CLOCK_TIMER);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffect_002, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffect_002 in");
    bool state { false };
    int32_t ret = IsSupportEffect("haptic.xxx.yyy", &state);
    ASSERT_EQ(ret, 0);
    ASSERT_FALSE(state);
}

HWTEST_F(VibratorAgentTest, IsSupportEffect_003, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffect_003 in");
    bool state { false };
    int32_t ret = IsSupportEffect(nullptr, &state);
    ASSERT_NE(ret, 0);
    ASSERT_FALSE(state);
}

HWTEST_F(VibratorAgentTest, IsSupportEffect_004, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffect_004 in");
    bool state { false };
    int32_t ret = IsSupportEffect(VIBRATOR_TYPE_FAIL, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibrator(VIBRATOR_TYPE_FAIL);
        ASSERT_EQ(ret, 0);
        Cancel();
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_FAIL);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffect_005, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffect_005 in");
    bool state { false };
    int32_t ret = IsSupportEffect(VIBRATOR_TYPE_CHARGING, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibrator(VIBRATOR_TYPE_CHARGING);
        ASSERT_EQ(ret, 0);
        Cancel();
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_CHARGING);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffect_006, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffect_006 in");
    bool state { false };
    int32_t ret = IsSupportEffect(VIBRATOR_TYPE_LONG_PRESS_HEAVY, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibrator(VIBRATOR_TYPE_LONG_PRESS_HEAVY);
        ASSERT_EQ(ret, 0);
        Cancel();
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_LONG_PRESS_HEAVY);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffect_007, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffect_007 in");
    bool state { false };
    int32_t ret = IsSupportEffect(VIBRATOR_TYPE_LONG_PRESS_LIGHT, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibrator(VIBRATOR_TYPE_LONG_PRESS_LIGHT);
        ASSERT_EQ(ret, 0);
        Cancel();
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_LONG_PRESS_LIGHT);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffect_008, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffect_008 in");
    bool state { false };
    int32_t ret = IsSupportEffect(VIBRATOR_TYPE_LONG_PRESS_MEDIUM, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibrator(VIBRATOR_TYPE_LONG_PRESS_MEDIUM);
        ASSERT_EQ(ret, 0);
        Cancel();
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_LONG_PRESS_MEDIUM);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffect_009, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffect_009 in");
    bool state { false };
    int32_t ret = IsSupportEffect(VIBRATOR_TYPE_SLIDE_LIGHT, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibrator(VIBRATOR_TYPE_SLIDE_LIGHT);
        ASSERT_EQ(ret, 0);
        Cancel();
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_SLIDE_LIGHT);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffect_010, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffect_010 in");
    bool state { false };
    int32_t ret = IsSupportEffect(VIBRATOR_TYPE_THRESHOID, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibrator(VIBRATOR_TYPE_THRESHOID);
        ASSERT_EQ(ret, 0);
        Cancel();
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_THRESHOID);
    }
}

HWTEST_F(VibratorAgentTest, GetDelayTime_001, TestSize.Level1)
{
    MISC_HILOGI("GetDelayTime_001 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        int32_t delayTime { -1 };
        int32_t ret = GetDelayTime(delayTime);
        ASSERT_EQ(ret, 0);
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PreProcess_001, TestSize.Level1)
{
    MISC_HILOGI("PreProcess_001 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("invalid_file_name");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        struct stat64 statbuf = { 0 };
        bool ret = fstat64(fileDescriptor.fd, &statbuf);
        if (ret) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            ret = PreProcess(vfd, package);
            ASSERT_NE(ret, 0);
            ret = FreeVibratorPackage(package);
            ASSERT_NE(ret, 0);
        } else {
            ASSERT_EQ(isSupport, false);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PreProcess_002, TestSize.Level1)
{
    MISC_HILOGI("PreProcess_002 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_duration.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            int32_t ret = PreProcess(vfd, package);
            ASSERT_NE(ret, 0);
            ret = FreeVibratorPackage(package);
            ASSERT_NE(ret, 0);
        } else {
            ASSERT_FALSE(true);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PreProcess_003, TestSize.Level1)
{
    MISC_HILOGI("PreProcess_003 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_frequency.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            int32_t ret = PreProcess(vfd, package);
            ASSERT_NE(ret, 0);
            ret = FreeVibratorPackage(package);
            ASSERT_NE(ret, 0);
        } else {
            ASSERT_FALSE(true);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PreProcess_004, TestSize.Level1)
{
    MISC_HILOGI("PreProcess_004 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_intensity.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            int32_t ret = PreProcess(vfd, package);
            ASSERT_NE(ret, 0);
            ret = FreeVibratorPackage(package);
            ASSERT_NE(ret, 0);
        } else {
            ASSERT_FALSE(true);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PreProcess_005, TestSize.Level1)
{
    MISC_HILOGI("PreProcess_005 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_startTime.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            int32_t ret = PreProcess(vfd, package);
            ASSERT_NE(ret, 0);
            ret = FreeVibratorPackage(package);
            ASSERT_NE(ret, 0);
        } else {
            ASSERT_FALSE(true);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PreProcess_006, TestSize.Level1)
{
    MISC_HILOGI("PreProcess_006 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_type.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            int32_t ret = PreProcess(vfd, package);
            ASSERT_NE(ret, 0);
            ret = FreeVibratorPackage(package);
            ASSERT_NE(ret, 0);
        } else {
            ASSERT_FALSE(true);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PreProcess_007, TestSize.Level1)
{
    MISC_HILOGI("PreProcess_007 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_event_overlap_1.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            int32_t ret = PreProcess(vfd, package);
            ASSERT_EQ(ret, 0);
            ret = FreeVibratorPackage(package);
            ASSERT_EQ(ret, 0);
        } else {
            ASSERT_FALSE(true);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PreProcess_008, TestSize.Level1)
{
    MISC_HILOGI("PreProcess_008 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_event_overlap_2.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            int32_t ret = PreProcess(vfd, package);
            ASSERT_EQ(ret, 0);
            ret = FreeVibratorPackage(package);
            ASSERT_EQ(ret, 0);
        } else {
            ASSERT_FALSE(true);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PreProcess_009, TestSize.Level1)
{
    MISC_HILOGI("PreProcess_009 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_129_event.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            int32_t ret = PreProcess(vfd, package);
            ASSERT_NE(ret, 0);
            ret = FreeVibratorPackage(package);
            ASSERT_NE(ret, 0);
        } else {
            ASSERT_FALSE(true);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayPattern_001, TestSize.Level1)
{
    MISC_HILOGI("PlayPattern_001 in");
    bool isSupport = IsSupportVibratorCustom();
    if (isSupport) {
        int32_t delayTime { -1 };
        int32_t ret = GetDelayTime(delayTime);
        ASSERT_EQ(ret, 0);
        MISC_HILOGD("delayTime:%{public}d", delayTime);
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            ret = PreProcess(vfd, package);
            ASSERT_EQ(ret, 0);
            for (int32_t i = 0; i < package.patternNum; ++i) {
                if (i == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(package.patterns[i].time));
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(package.patterns[i].time) -
                        std::chrono::milliseconds(package.patterns[i - 1].time));
                }
                ASSERT_EQ(SetUsage(USAGE_UNKNOWN), true);
                MISC_HILOGD("pointNum:%{public}d", package.patterns[i].events[i].pointNum);
                ret = PlayPattern(package.patterns[i]);
                ASSERT_EQ(ret, 0);
            }
        }
        ret = FreeVibratorPackage(package);
        ASSERT_EQ(ret, 0);
        Cancel();
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayPrimitiveEffect_001, TestSize.Level1)
{
    MISC_HILOGI("PlayPrimitiveEffect_001 in");
    int32_t ret = PlayPrimitiveEffect(nullptr, INTENSITY_HIGH);
    ASSERT_EQ(ret, PARAMETER_ERROR);
}

HWTEST_F(VibratorAgentTest, PlayPrimitiveEffect_002, TestSize.Level1)
{
    MISC_HILOGI("PlayPrimitiveEffect_002 in");
    bool state { false };
    int32_t ret = IsSupportEffect(VIBRATOR_TYPE_SLIDE, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = PlayPrimitiveEffect(VIBRATOR_TYPE_SLIDE, INTENSITY_INVALID);
        ASSERT_EQ(ret, PARAMETER_ERROR);
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_SLIDE);
    }
}

HWTEST_F(VibratorAgentTest, PlayPrimitiveEffect_003, TestSize.Level1)
{
    MISC_HILOGI("PlayPrimitiveEffect_003 in");
    bool state { false };
    int32_t ret = IsSupportEffect(VIBRATOR_TYPE_SLIDE, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = PlayPrimitiveEffect(VIBRATOR_TYPE_SLIDE, INTENSITY_LOW);
        ASSERT_EQ(ret, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
        Cancel();
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_SLIDE);
    }
}

HWTEST_F(VibratorAgentTest, PlayPrimitiveEffect_004, TestSize.Level1)
{
    MISC_HILOGI("PlayPrimitiveEffect_004 in");
    bool state { false };
    int32_t ret = IsSupportEffect(VIBRATOR_TYPE_SLIDE, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = PlayPrimitiveEffect(VIBRATOR_TYPE_SLIDE, INTENSITY_MEDIUM);
        ASSERT_EQ(ret, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
        Cancel();
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_SLIDE);
    }
}

HWTEST_F(VibratorAgentTest, PlayPrimitiveEffect_005, TestSize.Level1)
{
    MISC_HILOGI("PlayPrimitiveEffect_005 in");
    bool state { false };
    int32_t ret = IsSupportEffect(VIBRATOR_TYPE_SLIDE, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = PlayPrimitiveEffect(VIBRATOR_TYPE_SLIDE, INTENSITY_HIGH);
        ASSERT_EQ(ret, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
        Cancel();
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_SLIDE);
    }
}

HWTEST_F(VibratorAgentTest, IsHdHapticSupported_001, TestSize.Level1)
{
    MISC_HILOGI("IsHdHapticSupported_001 in");
    bool isSupport = (IsSupportVibratorCustom() && IsHdHapticSupported());
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    Cancel();
}

HWTEST_F(VibratorAgentTest, StartVibratorUseNotifactionTest, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorUseNotifactionTest in");
    bool flag = SetUsage(USAGE_UNKNOWN);
    ASSERT_TRUE(flag);
    int32_t ret = StartVibrator(VIBRATOR_TYPE_FAIL);
    ASSERT_EQ(ret, SUCCESS);
    flag = SetUsage(USAGE_NOTIFICATION);
    ASSERT_TRUE(flag);
    ret = StartVibrator(VIBRATOR_TYPE_FAIL);
    ASSERT_TRUE(ret == SUCCESS || ret == DEVICE_OPERATION_FAILED);
    Cancel();
}

HWTEST_F(VibratorAgentTest, StartVibratorUseRingTest, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorUseRingTest in");
    bool flag = SetUsage(USAGE_UNKNOWN);
    ASSERT_TRUE(flag);
    int32_t ret = StartVibrator(VIBRATOR_TYPE_FAIL);
    ASSERT_EQ(ret, SUCCESS);
    flag = SetUsage(USAGE_RING);
    ASSERT_TRUE(flag);
    ret = StartVibrator(VIBRATOR_TYPE_FAIL);
    ASSERT_TRUE(ret == SUCCESS || ret == DEVICE_OPERATION_FAILED);
    Cancel();
}

HWTEST_F(VibratorAgentTest, GetVibrateModeTest, TestSize.Level1)
{
    MISC_HILOGI("GetVibrateModeTest in");
    VibratorCapacity vibrateCapacity;
    int32_t mode = vibrateCapacity.GetVibrateMode();
    ASSERT_EQ(mode, ERROR);
    vibrateCapacity.isSupportHdHaptic = true;
    mode = vibrateCapacity.GetVibrateMode();
    ASSERT_EQ(mode, VIBRATE_MODE_HD);
    vibrateCapacity.isSupportHdHaptic = false;
    vibrateCapacity.isSupportPresetMapping = true;
    mode = vibrateCapacity.GetVibrateMode();
    ASSERT_EQ(mode, VIBRATE_MODE_MAPPING);
    vibrateCapacity.isSupportHdHaptic = false;
    vibrateCapacity.isSupportPresetMapping = false;
    vibrateCapacity.isSupportTimeDelay = true;
    mode = vibrateCapacity.GetVibrateMode();
    ASSERT_EQ(mode, VIBRATE_MODE_TIMES);
}

HWTEST_F(VibratorAgentTest, VibratePatternTest, TestSize.Level1)
{
    MISC_HILOGI("VibratePatternTest in");
    VibratePattern vibratePattern;
    vibratePattern.startTime = 10;
    vibratePattern.patternDuration = 1000;
    VibrateEvent vibrateEvent;
    vibrateEvent.time = 1;
    vibrateEvent.duration = 50;
    vibrateEvent.intensity = 98;
    vibrateEvent.frequency = 78;
    vibrateEvent.index = 0;
    vibrateEvent.tag = EVENT_TAG_CONTINUOUS;
    VibrateCurvePoint vibrateCurvePoint;
    vibrateCurvePoint.time = 1;
    vibrateCurvePoint.intensity = 1;
    vibrateCurvePoint.frequency = 1;
    vibrateEvent.points.push_back(vibrateCurvePoint);
    vibratePattern.events.push_back(vibrateEvent);
    Parcel data;
    bool flag = vibratePattern.Marshalling(data);
    ASSERT_EQ(flag, true);
    VibratePattern *ret = vibratePattern.Unmarshalling(data);
    ASSERT_EQ(ret->startTime, 10);
    ASSERT_EQ(ret->patternDuration, 1000);
    ret = nullptr;
}

bool IsSupportVibratorEffectEnhanced(const VibratorIdentifier identifier, const char* effectId)
{
    bool state { false };
    IsSupportEffectEnhanced(identifier, effectId, &state);
    return state;
}

HWTEST_F(VibratorAgentTest, StartVibratorEnhancedTest_001, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorEnhancedTest_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorEffectEnhanced(identifier, VIBRATOR_TYPE_CLOCK_TIMER);
    if (isSupport) {
        int32_t ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_CLOCK_TIMER);
        ASSERT_EQ(ret, 0);
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, StartVibratorEnhancedTest_002, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorEnhancedTest_002 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = StartVibratorEnhanced(identifier, "");
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StartVibratorEnhancedTest_003, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorTest_003 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = StartVibratorEnhanced(identifier, nullptr);
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StartVibratorOnceEnhancedTest_001, TestSize.Level0)
{
    MISC_HILOGI("StartVibratorOnceEnhancedTest_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = StartVibratorOnceEnhanced(identifier, 300);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, StartVibratorOnceEnhancedTest_002, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorOnceEnhancedTest_002 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = StartVibratorOnceEnhanced(identifier, 0);
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StartVibratorOnceEnhancedTest_003, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorOnceEnhancedTest_003 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = StartVibratorOnceEnhanced(identifier, 1800000);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, StartVibratorOnceEnhancedTest_004, TestSize.Level1)
{
    MISC_HILOGI("StartVibratorOnceEnhancedTest_004 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = StartVibratorOnceEnhanced(identifier, 1800001);
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StopVibratorEnhancedTest_001, TestSize.Level1)
{
    MISC_HILOGI("StopVibratorEnhancedTest_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = StopVibratorEnhanced(identifier, "time");
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, StopVibratorEnhancedTest_002, TestSize.Level1)
{
    MISC_HILOGI("StopVibratorEnhancedTest_002 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = StopVibratorEnhanced(identifier, "preset");
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StopVibratorEnhancedTest_003, TestSize.Level1)
{
    MISC_HILOGI("StopVibratorEnhancedTest_003 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = StopVibratorEnhanced(identifier, "");
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StopVibratorEnhancedTest_004, TestSize.Level1)
{
    MISC_HILOGI("StopVibratorEnhancedTest_004 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = StopVibratorEnhanced(identifier, nullptr);
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, StopVibratorEnhancedTest_005, TestSize.Level1)
{
    MISC_HILOGI("StopVibratorEnhancedTest_005 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = StartVibratorOnceEnhanced(identifier, 300);
    ASSERT_EQ(ret, 0);
    ret = StopVibratorEnhanced(identifier, "time");
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, SetLoopCountEnhanced_001, TestSize.Level1)
{
    MISC_HILOGI("SetLoopCountEnhanced_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool ret = SetLoopCountEnhanced(identifier, 300);
    ASSERT_TRUE(ret);
}

HWTEST_F(VibratorAgentTest, SetLoopCountEnhanced_002, TestSize.Level1)
{
    MISC_HILOGI("SetLoopCount_002 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool ret = SetLoopCountEnhanced(identifier, -1);
    ASSERT_FALSE(ret);
}

HWTEST_F(VibratorAgentTest, SetLoopCountEnhanced_003, TestSize.Level1)
{
    MISC_HILOGI("SetLoopCountEnhanced_003 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool ret = SetLoopCountEnhanced(identifier, 0);
    ASSERT_FALSE(ret);
}

HWTEST_F(VibratorAgentTest, SetUsageEnhanced_001, TestSize.Level1)
{
    MISC_HILOGI("SetUsageEnhanced_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool ret = SetUsageEnhanced(identifier, 0);
    ASSERT_TRUE(ret);
}

HWTEST_F(VibratorAgentTest, SetUsageEnhanced_002, TestSize.Level1)
{
    MISC_HILOGI("SetUsageEnhanced_002 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool ret = SetUsageEnhanced(identifier, -1);
    ASSERT_FALSE(ret);
}

HWTEST_F(VibratorAgentTest, SetUsageEnhanced_003, TestSize.Level1)
{
    MISC_HILOGI("SetUsage_003 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool ret = SetUsageEnhanced(identifier, USAGE_MAX);
    ASSERT_FALSE(ret);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_001, TestSize.Level0)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_002, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_002 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_003, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_003 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = (IsSupportVibratorCustomEnhanced(identifier) &&
        IsSupportVibratorEffectEnhanced(identifier, VIBRATOR_TYPE_FAIL));
    if (isSupport) {
        bool flag = SetLoopCountEnhanced(identifier, 2);
        ASSERT_TRUE(flag);
        int32_t ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_FAIL);
        ASSERT_EQ(ret, 0);
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_004, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_004 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = (IsSupportVibratorCustomEnhanced(identifier) &&
        IsSupportVibratorEffectEnhanced(identifier, VIBRATOR_TYPE_FAIL));
    if (isSupport) {
        bool flag = SetUsageEnhanced(identifier, USAGE_ALARM);
        ASSERT_TRUE(flag);
        int32_t ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_FAIL);
        ASSERT_EQ(ret, 0);
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_005, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_005 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = (IsSupportVibratorCustomEnhanced(identifier) &&
        IsSupportVibratorEffectEnhanced(identifier, VIBRATOR_TYPE_FAIL));
    if (isSupport) {
        bool flag = SetUsageEnhanced(identifier, USAGE_UNKNOWN);
        ASSERT_TRUE(flag);
        int32_t ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_FAIL);
        ASSERT_EQ(ret, 0);
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_006, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_006 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = (IsSupportVibratorCustomEnhanced(identifier) &&
        IsSupportVibratorEffectEnhanced(identifier, VIBRATOR_TYPE_FAIL));
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            bool flag = SetUsageEnhanced(identifier, USAGE_ALARM);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_FAIL);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_007, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_007 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = (IsSupportVibratorCustomEnhanced(identifier) &&
        IsSupportVibratorEffectEnhanced(identifier, VIBRATOR_TYPE_FAIL));
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            bool flag = SetUsageEnhanced(identifier, USAGE_UNKNOWN);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_FAIL);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_008, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_008 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        bool flag = SetUsageEnhanced(identifier, USAGE_ALARM);
        ASSERT_TRUE(flag);
        int32_t ret = StartVibratorOnceEnhanced(identifier, 500);
        ASSERT_EQ(ret, 0);
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_009, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_009 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        bool flag = SetUsage(USAGE_UNKNOWN);
        ASSERT_TRUE(flag);
        int32_t ret = StartVibratorOnceEnhanced(identifier, 500);
        ASSERT_EQ(ret, 0);
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_010, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_010 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            bool flag = SetUsageEnhanced(identifier, USAGE_ALARM);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = StartVibratorOnceEnhanced(identifier, 500);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_011, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_011 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            bool flag = SetUsageEnhanced(identifier, USAGE_UNKNOWN);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = StartVibratorOnceEnhanced(identifier, 500);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_012, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_012 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_128_event.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_013, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_013 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_type.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_014, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_014 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_startTime.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_015, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_015 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_duration.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_016, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_016 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_intensity.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_017, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_017 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_invalid_frequency.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_018, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_018 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_129_event.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_019, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_019 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_big_file_size.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_020, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_020 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_event_overlap_1.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_021, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_021 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/test_event_overlap_2.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_022, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_022 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/Jet_N2O.he");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayVibratorCustomEnhanced_023, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustomEnhanced_023 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/Racing_Start.he");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, SetParametersEnhanced_001, TestSize.Level1)
{
    MISC_HILOGI("SetParametersEnhanced_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    VibratorParameter parameter = {
        .intensity = -1,
        .frequency = -15
    };
    bool ret = SetParametersEnhanced(identifier, parameter);
    ASSERT_FALSE(ret);
}

HWTEST_F(VibratorAgentTest, SetParametersEnhanced_002, TestSize.Level1)
{
    MISC_HILOGI("SetParametersEnhanced_002 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    VibratorParameter parameter = {
        .intensity = 70,
        .frequency = 150
    };
    bool ret = SetParametersEnhanced(identifier, parameter);
    ASSERT_FALSE(ret);
}

HWTEST_F(VibratorAgentTest, SetParametersEnhanced_003, TestSize.Level1)
{
    MISC_HILOGI("SetParametersEnhanced_003 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            VibratorParameter parameter = {
                .intensity = 50,
                .frequency = -15
            };
            bool flag = SetParametersEnhanced(identifier, parameter);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, SetParametersEnhanced_004, TestSize.Level1)
{
    MISC_HILOGI("SetParametersEnhanced_004 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/on_carpet.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            VibratorParameter parameter = {
                .intensity = 33,
                .frequency = 55
            };
            bool flag = SetParametersEnhanced(identifier, parameter);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, CancelEnhanced_001, TestSize.Level1)
{
    MISC_HILOGI("CancelEnhanced_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = CancelEnhanced(identifier);
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorAgentTest, CancelEnhanced_002, TestSize.Level1)
{
    MISC_HILOGI("CancelEnhanced_002 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier, fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = CancelEnhanced(identifier);
            ASSERT_EQ(ret, 0);
        }
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, CancelEnhanced_003, TestSize.Level1)
{
    MISC_HILOGI("CancelEnhanced_003 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = StartVibratorOnceEnhanced(identifier, 500);
    ASSERT_EQ(ret, 0);
    ret = CancelEnhanced(identifier);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, CancelEnhanced_004, TestSize.Level1)
{
    MISC_HILOGI("CancelEnhanced_004 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    if (IsSupportVibratorEffectEnhanced(identifier, VIBRATOR_TYPE_FAIL)) {
        int32_t ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_FAIL);
        ASSERT_EQ(ret, 0);
        ret = CancelEnhanced(identifier);
        ASSERT_EQ(ret, 0);
    }
    ASSERT_TRUE(true);
}

HWTEST_F(VibratorAgentTest, IsSupportEffectEnhanced_001, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffectEnhanced_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, VIBRATOR_TYPE_CLOCK_TIMER, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_CLOCK_TIMER);
        ASSERT_EQ(ret, 0);
        CancelEnhanced(identifier);
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_CLOCK_TIMER);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffectEnhanced_002, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffectEnhanced_002 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, "haptic.xxx.yyy", &state);
    ASSERT_EQ(ret, 0);
    ASSERT_FALSE(state);
}

HWTEST_F(VibratorAgentTest, IsSupportEffectEnhanced_003, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffectEnhanced_003 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, nullptr, &state);
    ASSERT_NE(ret, 0);
    ASSERT_FALSE(state);
}

HWTEST_F(VibratorAgentTest, IsSupportEffectEnhanced_004, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffectEnhanced_004 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, VIBRATOR_TYPE_FAIL, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_FAIL);
        ASSERT_EQ(ret, 0);
        CancelEnhanced(identifier);
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_FAIL);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffectEnhanced_005, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffectEnhanced_005 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, VIBRATOR_TYPE_CHARGING, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_CHARGING);
        ASSERT_EQ(ret, 0);
        CancelEnhanced(identifier);
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_CHARGING);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffectEnhanced_006, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffectEnhanced_006 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, VIBRATOR_TYPE_LONG_PRESS_HEAVY, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_LONG_PRESS_HEAVY);
        ASSERT_EQ(ret, 0);
        CancelEnhanced(identifier);
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_LONG_PRESS_HEAVY);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffectEnhanced_007, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffectEnhanced_007 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, VIBRATOR_TYPE_LONG_PRESS_LIGHT, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_LONG_PRESS_LIGHT);
        ASSERT_EQ(ret, 0);
        CancelEnhanced(identifier);
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_LONG_PRESS_LIGHT);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffectEnhanced_008, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffectEnhanced_008 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, VIBRATOR_TYPE_LONG_PRESS_MEDIUM, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_LONG_PRESS_MEDIUM);
        ASSERT_EQ(ret, 0);
        CancelEnhanced(identifier);
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_LONG_PRESS_MEDIUM);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffectEnhanced_009, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffectEnhanced_009 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, VIBRATOR_TYPE_SLIDE_LIGHT, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_SLIDE_LIGHT);
        ASSERT_EQ(ret, 0);
        CancelEnhanced(identifier);
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_SLIDE_LIGHT);
    }
}

HWTEST_F(VibratorAgentTest, IsSupportEffectEnhanced_010, TestSize.Level1)
{
    MISC_HILOGI("IsSupportEffectEnhanced_010 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, VIBRATOR_TYPE_THRESHOID, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = StartVibratorEnhanced(identifier, VIBRATOR_TYPE_THRESHOID);
        ASSERT_EQ(ret, 0);
        CancelEnhanced(identifier);
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_THRESHOID);
    }
}

HWTEST_F(VibratorAgentTest, GetDelayTimeEnhanced_001, TestSize.Level1)
{
    MISC_HILOGI("GetDelayTimeEnhanced_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (isSupport) {
        int32_t delayTime { -1 };
        int32_t ret = GetDelayTimeEnhanced(identifier, delayTime);
        ASSERT_EQ(ret, 0);
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(VibratorAgentTest, PlayPatternEnhanced_001, TestSize.Level1)
{
    MISC_HILOGI("PlayPatternEnhanced_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);

    if (!isSupport) {
        ASSERT_EQ(isSupport, false);
    }
    int32_t delayTime { -1 };
    int32_t ret = GetDelayTimeEnhanced(identifier, delayTime);
    ASSERT_EQ(ret, 0);
    MISC_HILOGD("delayTime:%{public}d", delayTime);
    FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop.json");
    MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
    VibratorFileDescription vfd;
    VibratorPackage package;
    struct stat64 statbuf = { 0 };
    if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
        vfd.fd = fileDescriptor.fd;
        vfd.offset = 0;
        vfd.length = statbuf.st_size;
        ret = PreProcess(vfd, package);
        ASSERT_EQ(ret, 0);
        for (int32_t i = 0; i < package.patternNum; ++i) {
            if (i == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(package.patterns[i].time));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(package.patterns[i].time) -
                    std::chrono::milliseconds(package.patterns[i - 1].time));
            }
            ASSERT_EQ(SetUsageEnhanced(identifier, USAGE_UNKNOWN), true);
            MISC_HILOGD("pointNum:%{public}d", package.patterns[i].events[i].pointNum);
            ret = PlayPatternEnhanced(identifier, package.patterns[i]);
            ASSERT_EQ(ret, 0);
        }
    }
    ret = FreeVibratorPackage(package);
    ASSERT_EQ(ret, 0);
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, PlayPrimitiveEffectEnhanced_001, TestSize.Level1)
{
    MISC_HILOGI("PlayPrimitiveEffectEnhanced_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    int32_t ret = PlayPrimitiveEffectEnhanced(identifier, nullptr, INTENSITY_HIGH);
    ASSERT_EQ(ret, PARAMETER_ERROR);
}

HWTEST_F(VibratorAgentTest, PlayPrimitiveEffectEnhanced_002, TestSize.Level1)
{
    MISC_HILOGI("PlayPrimitiveEffectEnhanced_002 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, VIBRATOR_TYPE_SLIDE, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = PlayPrimitiveEffectEnhanced(identifier, VIBRATOR_TYPE_SLIDE, INTENSITY_INVALID);
        ASSERT_EQ(ret, PARAMETER_ERROR);
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_SLIDE);
    }
}

HWTEST_F(VibratorAgentTest, PlayPrimitiveEffectEnhanced_003, TestSize.Level1)
{
    MISC_HILOGI("PlayPrimitiveEffectEnhanced_003 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, VIBRATOR_TYPE_SLIDE, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = PlayPrimitiveEffectEnhanced(identifier, VIBRATOR_TYPE_SLIDE, INTENSITY_LOW);
        ASSERT_EQ(ret, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
        CancelEnhanced(identifier);
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_SLIDE);
    }
}

HWTEST_F(VibratorAgentTest, PlayPrimitiveEffectEnhanced_004, TestSize.Level1)
{
    MISC_HILOGI("PlayPrimitiveEffectEnhanced_004 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, VIBRATOR_TYPE_SLIDE, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = PlayPrimitiveEffectEnhanced(identifier, VIBRATOR_TYPE_SLIDE, INTENSITY_MEDIUM);
        ASSERT_EQ(ret, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
        CancelEnhanced(identifier);
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_SLIDE);
    }
}

HWTEST_F(VibratorAgentTest, PlayPrimitiveEffectEnhanced_005, TestSize.Level1)
{
    MISC_HILOGI("PlayPrimitiveEffectEnhanced_005 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool state { false };
    int32_t ret = IsSupportEffectEnhanced(identifier, VIBRATOR_TYPE_SLIDE, &state);
    ASSERT_EQ(ret, 0);
    if (state) {
        ret = PlayPrimitiveEffectEnhanced(identifier, VIBRATOR_TYPE_SLIDE, INTENSITY_HIGH);
        ASSERT_EQ(ret, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
        CancelEnhanced(identifier);
    } else {
        MISC_HILOGI("Do not support %{public}s", VIBRATOR_TYPE_SLIDE);
    }
}

HWTEST_F(VibratorAgentTest, IsHdHapticSupportedEnhanced_001, TestSize.Level1)
{
    MISC_HILOGI("IsHdHapticSupportedEnhanced_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = (IsSupportVibratorCustomEnhanced(identifier) && IsHdHapticSupportedEnhanced(identifier));
    if (isSupport) {
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop.json");
        MISC_HILOGD("Test fd:%{public}d", fileDescriptor.fd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustomEnhanced(identifier,  fileDescriptor.fd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(isSupport, false);
    }
    CancelEnhanced(identifier);
}

HWTEST_F(VibratorAgentTest, GetVibratorIdList_001, TestSize.Level1)
{
    MISC_HILOGI("GetVibratorIdList_001 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    std::vector<VibratorInfos> result;
    int32_t ret = GetVibratorList(identifier, result);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(VibratorAgentTest, GetVibratorIdList_002, TestSize.Level1)
{
    MISC_HILOGI("GetVibratorIdList_002 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = 1,
    };
    std::vector<VibratorInfos> result;
    int32_t ret = GetVibratorList(identifier, result);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(VibratorAgentTest, GetVibratorIdList_003, TestSize.Level1)
{
    MISC_HILOGI("GetVibratorIdList_003 in");
    VibratorIdentifier identifier = {
        .deviceId = 1,
        .vibratorId = -1,
    };
    std::vector<VibratorInfos> result;
    int32_t ret = GetVibratorList(identifier, result);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(VibratorAgentTest, GetVibratorIdList_004, TestSize.Level1)
{
    MISC_HILOGI("GetVibratorIdList_004 in");
    VibratorIdentifier identifier = {
        .deviceId = 1,
        .vibratorId = 1,
    };
    std::vector<VibratorInfos> result;
    int32_t ret = GetVibratorList(identifier, result);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(VibratorAgentTest, GetVibratorIdList_005, TestSize.Level1)
{
    MISC_HILOGI("GetVibratorIdList_005 in");
    VibratorIdentifier identifier = {
        .deviceId = -9999,
        .vibratorId = 1,
    };
    std::vector<VibratorInfos> result;
    int32_t ret = GetVibratorList(identifier, result);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(VibratorAgentTest, GetEffectInfo_001, TestSize.Level1)
{
    MISC_HILOGI("GetEffectInfo_001 in");
    VibratorIdentifier identifier = {
        .deviceId = 1,
        .vibratorId = -1,
    };
    std::string effectType = "";
    EffectInfo result;
    int32_t ret = GetEffectInfo(identifier, effectType, result);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(VibratorAgentTest, GetEffectInfo_002, TestSize.Level1)
{
    MISC_HILOGI("GetEffectInfo_002 in");
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    std::string effectType = "";
    EffectInfo result;
    int32_t ret = GetEffectInfo(identifier, effectType, result);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(VibratorAgentTest, SubscribeVibrator_001, TestSize.Level1)
{
    MISC_HILOGI("SubscribeVibrator_001 in");
    int32_t ret = SubscribeVibratorPlug(testUser);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(VibratorAgentTest, SubscribeVibrator_002, TestSize.Level1)
{
    VibratorUser invalidUser = {
        .callback = nullptr,
    };
    MISC_HILOGI("SubscribeVibrator_002 in");
    int32_t ret = SubscribeVibratorPlug(invalidUser);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(VibratorAgentTest, UnSubscribeVibrator_001, TestSize.Level1)
{
    MISC_HILOGI("UnSubscribeVibrator_001 in");
    int32_t ret = UnSubscribeVibratorPlug(testUser);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
    ret = UnSubscribeVibratorPlug(testUser);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}
} // namespace Sensors
} // namespace OHOS