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

#include <gtest/gtest.h>
#include <thread>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

#include "vibrator_agent.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace Security::AccessToken;
using Security::AccessToken::AccessTokenID;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "VibratorAgentTest" };
PermissionDef infoManagerTestPermDef_ = {
    .permissionName = "ohos.permission.VIBRATE",
    .bundleName = "accesstoken_test",
    .grantMode = 1,
    .label = "label",
    .labelId = 1,
    .description = "test vibrator agent",
    .descriptionId = 1,
    .availableLevel = APL_NORMAL
};

PermissionStateFull infoManagerTestState_ = {
    .grantFlags = {1},
    .grantStatus = {PermissionState::PERMISSION_GRANTED},
    .isGeneral = true,
    .permissionName = "ohos.permission.VIBRATE",
    .resDeviceID = {"local"}
};

HapPolicyParams infoManagerTestPolicyPrams_ = {
    .apl = APL_NORMAL,
    .domain = "test.domain",
    .permList = {infoManagerTestPermDef_},
    .permStateList = {infoManagerTestState_}
};

HapInfoParams infoManagerTestInfoParms_ = {
    .bundleName = "vibratoragent_test",
    .userID = 1,
    .instIndex = 0,
    .appIDDesc = "vibratorAgentTest"
};
}  // namespace

class VibratorAgentTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

private:
    static AccessTokenID tokenID_;
};

AccessTokenID VibratorAgentTest::tokenID_ = 0;

void VibratorAgentTest::SetUpTestCase()
{
    AccessTokenIDEx tokenIdEx = {0};
    tokenIdEx = AccessTokenKit::AllocHapToken(infoManagerTestInfoParms_, infoManagerTestPolicyPrams_);
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
{}

HWTEST_F(VibratorAgentTest, StartVibratorTest_001, TestSize.Level1)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    int32_t ret = StartVibrator("haptic.clock.timer");
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, StartVibratorTest_002, TestSize.Level1)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    int32_t ret = StartVibrator("");
    ASSERT_EQ(ret, -1);
}

HWTEST_F(VibratorAgentTest, StartVibratorTest_003, TestSize.Level1)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    int32_t ret = StartVibrator(nullptr);
    ASSERT_EQ(ret, -1);
}

HWTEST_F(VibratorAgentTest, StartVibratorOnceTest_001, TestSize.Level1)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    int32_t ret = StartVibratorOnce(300);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, StartVibratorOnceTest_002, TestSize.Level1)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    int32_t ret = StartVibratorOnce(0);
    ASSERT_EQ(ret, -1);
}

HWTEST_F(VibratorAgentTest, StartVibratorOnceTest_003, TestSize.Level1)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    int32_t ret = StartVibratorOnce(1800000);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, StartVibratorOnceTest_004, TestSize.Level1)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    int32_t ret = StartVibratorOnce(1800001);
    ASSERT_EQ(ret, -1);
}

HWTEST_F(VibratorAgentTest, StopVibratorTest_001, TestSize.Level1)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    int32_t ret = StopVibrator("time");
    ASSERT_EQ(ret, 0);
}

HWTEST_F(VibratorAgentTest, StopVibratorTest_002, TestSize.Level1)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    int32_t ret = StopVibrator("preset");
    ASSERT_EQ(ret, -1);
}

HWTEST_F(VibratorAgentTest, StopVibratorTest_003, TestSize.Level1)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    int32_t ret = StopVibrator("");
    ASSERT_EQ(ret, -1);
}

HWTEST_F(VibratorAgentTest, StopVibratorTest_004, TestSize.Level1)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    int32_t ret = StopVibrator(nullptr);
    ASSERT_EQ(ret, -1);
}

HWTEST_F(VibratorAgentTest, StopVibratorTest_005, TestSize.Level1)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    int32_t ret = StartVibratorOnce(300);
    ASSERT_EQ(ret, 0);
    ret = StopVibrator("time");
    ASSERT_EQ(ret, 0);
}
}  // namespace Sensors
}  // namespace OHOS
