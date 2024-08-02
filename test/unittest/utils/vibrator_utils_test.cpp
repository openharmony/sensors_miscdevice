/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include "token_setproc.h"

#include "json_parser.h"
#include "sensors_errors.h"
#include "he_vibrator_decoder.h"
#include "vibrator_agent.h"

#undef LOG_TAG
#define LOG_TAG "VibratorUtilsTest"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
using namespace Security::AccessToken;
using Security::AccessToken::AccessTokenID;

namespace {
constexpr int32_t TIME_WAIT_FOR_OP = 500;
constexpr int32_t TIME_WAIT_FOR_OP_TWO_HUNDRED = 200;

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
    .bundleName = "vibratorutils_test",
    .userID = 1,
    .instIndex = 0,
    .appIDDesc = "vibratorUtilsTest"
};
}  // namespace

class VibratorUtileTest : public testing::Test {
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
        rawFd = open(path.c_str(), O_RDONLY);
    }
    ~FileDescriptor()
    {
        close(rawFd);
    }
    int32_t rawFd;
};

AccessTokenID VibratorUtileTest::tokenID_ = 0;

void VibratorUtileTest::SetUpTestCase()
{
    AccessTokenIDEx tokenIdEx = {0};
    tokenIdEx = AccessTokenKit::AllocHapToken(g_infoManagerTestInfoParms, g_infoManagerTestPolicyPrams);
    tokenID_ = tokenIdEx.tokenIdExStruct.tokenID;
    ASSERT_NE(0, tokenID_);
    ASSERT_EQ(0, SetSelfTokenID(tokenID_));
}

void VibratorUtileTest::TearDownTestCase()
{
    int32_t ret = AccessTokenKit::DeleteToken(tokenID_);
    if (tokenID_ != 0) {
        ASSERT_EQ(RET_SUCCESS, ret);
    }
}

void VibratorUtileTest::SetUp()
{}

void VibratorUtileTest::TearDown()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_TWO_HUNDRED));
}

bool IsSupportVibratorEffect(const char* effectId)
{
    bool state { false };
    IsSupportEffect(effectId, &state);
    return state;
}

HWTEST_F(VibratorUtileTest, PlayVibratorCustom_001, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_001 in");
    if (IsSupportVibratorCustom()) {
        FileDescriptor fileDescriptor("/data/test/vibrator/car_crash.he");
        MISC_HILOGD("Test rawFd:%{public}d", fileDescriptor.rawFd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.rawFd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.rawFd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(0, 0);
    }
    Cancel();
}

HWTEST_F(VibratorUtileTest, PlayVibratorCustom_002, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_002 in");
    if (IsSupportVibratorCustom() && IsSupportVibratorEffect(VIBRATOR_TYPE_FAIL)) {
        bool flag = SetLoopCount(2);
        ASSERT_TRUE(flag);
        int32_t ret = StartVibrator(VIBRATOR_TYPE_FAIL);
        ASSERT_EQ(ret, 0);
        FileDescriptor fileDescriptor("/data/test/vibrator/car_crash.he");
        MISC_HILOGD("Test rawFd:%{public}d", fileDescriptor.rawFd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.rawFd, &statbuf) == 0) {
            ret = PlayVibratorCustom(fileDescriptor.rawFd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(0, 0);
    }
    Cancel();
}

HWTEST_F(VibratorUtileTest, PlayVibratorCustom_003, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_003 in");
    if (IsSupportVibratorCustom() && IsSupportVibratorEffect(VIBRATOR_TYPE_FAIL)) {
        FileDescriptor fileDescriptor("/data/test/vibrator/car_crash.he");
        MISC_HILOGD("Test rawFd:%{public}d", fileDescriptor.rawFd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.rawFd, &statbuf) == 0) {
            bool flag = SetUsage(USAGE_ALARM);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustom(fileDescriptor.rawFd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = StartVibrator(VIBRATOR_TYPE_FAIL);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(0, 0);
    }
    Cancel();
}

HWTEST_F(VibratorUtileTest, PlayVibratorCustom_004, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_004 in");
    if (IsSupportVibratorCustom()) {
        bool flag = SetUsage(USAGE_ALARM);
        ASSERT_TRUE(flag);
        int32_t ret = StartVibratorOnce(500);
        ASSERT_EQ(ret, 0);
        FileDescriptor fileDescriptor("/data/test/vibrator/car_crash.he");
        MISC_HILOGD("Test rawFd:%{public}d", fileDescriptor.rawFd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.rawFd, &statbuf) == 0) {
            ret = PlayVibratorCustom(fileDescriptor.rawFd, 0, statbuf.st_size);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(0, 0);
    }
    Cancel();
}

HWTEST_F(VibratorUtileTest, PlayVibratorCustom_005, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_005 in");
    if (IsSupportVibratorCustom()) {
        FileDescriptor fileDescriptor("/data/test/vibrator/car_crash.he");
        MISC_HILOGD("Test rawFd:%{public}d", fileDescriptor.rawFd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.rawFd, &statbuf) == 0) {
            bool flag = SetUsage(USAGE_ALARM);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustom(fileDescriptor.rawFd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = StartVibratorOnce(500);
            ASSERT_NE(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(0, 0);
    }
    Cancel();
}

HWTEST_F(VibratorUtileTest, PlayVibratorCustom_006, TestSize.Level1)
{
    MISC_HILOGI("PlayVibratorCustom_006 in");
    if (IsSupportVibratorCustom()) {
        FileDescriptor fileDescriptor("/data/test/vibrator/car_crash.he");
        MISC_HILOGD("Test rawFd:%{public}d", fileDescriptor.rawFd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.rawFd, &statbuf) == 0) {
            bool flag = SetUsage(USAGE_UNKNOWN);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustom(fileDescriptor.rawFd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = StartVibratorOnce(500);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(0, 0);
    }
    Cancel();
}

HWTEST_F(VibratorUtileTest, SetParameters_001, TestSize.Level1)
{
    MISC_HILOGI("SetParameters_001 in");
    if (IsSupportVibratorCustom()) {
        FileDescriptor fileDescriptor("/data/test/vibrator/car_crash.he");
        MISC_HILOGD("Test rawFd:%{public}d", fileDescriptor.rawFd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.rawFd, &statbuf) == 0) {
            VibratorParameter parameter = {
                .intensity = 50,
                .frequency = -15
            };
            bool flag = SetParameters(parameter);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustom(fileDescriptor.rawFd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(0, 0);
    }
    Cancel();
}

HWTEST_F(VibratorUtileTest, SetParameters_002, TestSize.Level1)
{
    MISC_HILOGI("SetParameters_002 in");
    if (IsSupportVibratorCustom()) {
        FileDescriptor fileDescriptor("/data/test/vibrator/car_crash.he");
        MISC_HILOGD("Test rawFd:%{public}d", fileDescriptor.rawFd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.rawFd, &statbuf) == 0) {
            VibratorParameter parameter = {
                .intensity = 33,
                .frequency = 55
            };
            bool flag = SetParameters(parameter);
            ASSERT_TRUE(flag);
            int32_t ret = PlayVibratorCustom(fileDescriptor.rawFd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    } else {
        ASSERT_EQ(0, 0);
    }
    Cancel();
}

HWTEST_F(VibratorUtileTest, Cancel_001, TestSize.Level1)
{
    MISC_HILOGI("Cancel_001 in");
    int32_t ret = Cancel();
    ASSERT_NE(ret, 0);
}

HWTEST_F(VibratorUtileTest, Cancel_002, TestSize.Level1)
{
    MISC_HILOGI("Cancel_002 in");
    if (IsSupportVibratorCustom()) {
        FileDescriptor fileDescriptor("/data/test/vibrator/car_crash.he");
        MISC_HILOGD("Test rawFd:%{public}d", fileDescriptor.rawFd);
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.rawFd, &statbuf) == 0) {
            int32_t ret = PlayVibratorCustom(fileDescriptor.rawFd, 0, statbuf.st_size);
            ASSERT_EQ(ret, 0);
            ret = Cancel();
            ASSERT_EQ(ret, 0);
        }
    } else {
        ASSERT_EQ(0, 0);
    }
}
}  // namespace Sensors
}  // namespace OHOS
