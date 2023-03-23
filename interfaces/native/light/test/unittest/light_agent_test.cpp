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

#include "light_agent.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
constexpr int32_t TIME_WAIT_FOR_OP = 2;
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "LightAgentTest" };
}  // namespace

class LightAgentTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

LightInfo *lightInfo_ = nullptr;
int32_t lightId_ = -1;
int32_t invalidLightId_ = -1;
int32_t lightType_ = -1;

/**
 * @tc.name: StartLightTest_001
 * @tc.desc: Verify GetLightList
 * @tc.type: FUNC
 * @tc.require: I63TFA
 */
HWTEST_F(LightAgentTest, StartLightTest_001, TestSize.Level1)
{
    CALL_LOG_ENTER;
    int32_t count = -1;
    int32_t ret = GetLightList(&lightInfo_, count);
    for (int32_t i = 0; i < count; ++i) {
        MISC_HILOGI("lightId: %{public}d, lightName: %{public}s, lightNumber: %{public}d, lightType: %{public}d",
            lightInfo_[i].lightId, lightInfo_[i].lightName, lightInfo_[i].lightNumber, lightInfo_[i].lightType);
        lightId_ = lightInfo_[i].lightId;
        lightType_ = lightInfo_[i].lightType;
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: StartLightTest_002
 * @tc.desc: Verify GetLightList
 * @tc.type: FUNC
 * @tc.require: I63TFA
 */
HWTEST_F(LightAgentTest, StartLightTest_002, TestSize.Level1)
{
    CALL_LOG_ENTER;
    int32_t count = -1;
    int32_t ret = GetLightList(nullptr, count);
    ASSERT_EQ(ret, -1);
}

bool GetLightColor(LightColor &color, int32_t lightType)
{
    switch (lightType) {
        case LIGHT_TYPE_SINGLE_COLOR: {
            color.singleColor = 0Xff;
            return true;
        }
        case LIGHT_TYPE_RGB_COLOR: {
            color.rgbColor = {
                .r = 0Xff,
                .g = 0Xff,
                .b = 0Xff
            };
            return true;
        }
        case LIGHT_TYPE_WRGB_COLOR: {
            color.wrgbColor = {
                .w = 0Xff,
                .r = 0Xff,
                .g = 0Xff,
                .b = 0Xff
            };
            return true;
        }
        default: {
            MISC_HILOGE("lightType: %{public}d invalid", lightType);
            return false;
        }
    }
}

/**
 * @tc.name: StartLightTest_003
 * @tc.desc: Verify TurnOn
 * @tc.type: FUNC
 * @tc.require: I63TFA
 */
HWTEST_F(LightAgentTest, StartLightTest_003, TestSize.Level1)
{
    CALL_LOG_ENTER;
    int32_t powerLightId = 1;
    TurnOff(powerLightId);
    LightColor color;
    bool ret = GetLightColor(color, lightType_);
    if (!ret) {
        ASSERT_EQ(ret, -1);
    } else {
        LightAnimation animation;
        animation.mode = LIGHT_MODE_DEFAULT;
        animation.onTime = 50;
        animation.offTime = 50;
        int32_t ret = TurnOn(lightId_, color, animation);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
        ASSERT_EQ(ret, 0);
    }
}

/**
 * @tc.name: StartLightTest_004
 * @tc.desc: Verify TurnOn
 * @tc.type: FUNC
 * @tc.require: I63TFA
 */
HWTEST_F(LightAgentTest, StartLightTest_004, TestSize.Level1)
{
    CALL_LOG_ENTER;
    LightColor color;
    bool ret = GetLightColor(color, lightType_);
    if (!ret) {
        ASSERT_EQ(ret, -1);
    } else {
        LightAnimation animation;
        animation.mode = LIGHT_MODE_BUTT;
        animation.onTime = 50;
        animation.offTime = 50;
        int32_t ret = TurnOn(lightId_, color, animation);
        ASSERT_EQ(ret, -1);
    }
}

/**
 * @tc.name: StartLightTest_005
 * @tc.desc: Verify TurnOn
 * @tc.type: FUNC
 * @tc.require: I63TFA
 */
HWTEST_F(LightAgentTest, StartLightTest_005, TestSize.Level1)
{
    CALL_LOG_ENTER;
    LightColor color;
    bool ret = GetLightColor(color, lightType_);
    if (!ret) {
        ASSERT_EQ(ret, -1);
    } else {
        LightAnimation animation;
        animation.mode = -1;
        animation.onTime = 50;
        animation.offTime = 50;
        int32_t ret = TurnOn(lightId_, color, animation);
        ASSERT_EQ(ret, -1);
    }
}

/**
 * @tc.name: StartLightTest_006
 * @tc.desc: Verify TurnOn
 * @tc.type: FUNC
 * @tc.require: I63TFA
 */
HWTEST_F(LightAgentTest, StartLightTest_006, TestSize.Level1)
{
    CALL_LOG_ENTER;
    LightColor color;
    bool ret = GetLightColor(color, lightType_);
    if (!ret) {
        ASSERT_EQ(ret, -1);
    } else {
        LightAnimation animation;
        animation.mode = LIGHT_MODE_DEFAULT;
        animation.onTime = -1;
        animation.offTime = 50;
        int32_t ret = TurnOn(lightId_, color, animation);
        ASSERT_EQ(ret, -1);
    }
}

/**
 * @tc.name: StartLightTest_007
 * @tc.desc: Verify TurnOn
 * @tc.type: FUNC
 * @tc.require: I63TFA
 */
HWTEST_F(LightAgentTest, StartLightTest_007, TestSize.Level1)
{
    CALL_LOG_ENTER;
    LightColor color;
    bool ret = GetLightColor(color, lightType_);
    if (!ret) {
        ASSERT_EQ(ret, -1);
    } else {
        LightAnimation animation;
        animation.mode = LIGHT_MODE_DEFAULT;
        animation.onTime = 50;
        animation.offTime = -1;
        int32_t ret = TurnOn(lightId_, color, animation);
        ASSERT_EQ(ret, -1);
    }
}

/**
 * @tc.name: StartLightTest_008
 * @tc.desc: Verify TurnOn
 * @tc.type: FUNC
 * @tc.require: I63TFA
 */
HWTEST_F(LightAgentTest, StartLightTest_008, TestSize.Level1)
{
    CALL_LOG_ENTER;
    LightColor color;
    bool ret = GetLightColor(color, lightType_);
    if (!ret) {
        ASSERT_EQ(ret, -1);
    } else {
        LightAnimation animation;
        animation.mode = LIGHT_MODE_DEFAULT;
        animation.onTime = 2;
        animation.offTime = 2;
        int32_t ret = TurnOn(lightId_, color, animation);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
        ASSERT_EQ(ret, 0);
    }
}

/**
 * @tc.name: StartLightTest_009
 * @tc.desc: Verify TurnOn
 * @tc.type: FUNC
 * @tc.require: I63TFA
 */
HWTEST_F(LightAgentTest, StartLightTest_009, TestSize.Level1)
{
    CALL_LOG_ENTER;
    LightColor color;
    bool ret = GetLightColor(color, lightType_);
    if (!ret) {
        ASSERT_EQ(ret, -1);
    } else {
        LightAnimation animation;
        animation.mode = LIGHT_MODE_DEFAULT;
        animation.onTime = 2;
        animation.offTime = 2;
        int32_t ret = TurnOn(invalidLightId_, color, animation);
        ASSERT_EQ(ret, -1);
    }
}

/**
 * @tc.name: StartLightTest_010
 * @tc.desc: Verify TurnOff
 * @tc.type: FUNC
 * @tc.require: I63TFA
 */
HWTEST_F(LightAgentTest, StartLightTest_010, TestSize.Level1)
{
    CALL_LOG_ENTER;
    int32_t ret = TurnOff(lightId_);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: StartLightTest_011
 * @tc.desc: Verify TurnOff
 * @tc.type: FUNC
 * @tc.require: I63TFA
 */
HWTEST_F(LightAgentTest, StartLightTest_011, TestSize.Level1)
{
    CALL_LOG_ENTER;
    int32_t ret = TurnOff(invalidLightId_);
    ASSERT_EQ(ret, -1);
}
}  // namespace Sensors
}  // namespace OHOS
