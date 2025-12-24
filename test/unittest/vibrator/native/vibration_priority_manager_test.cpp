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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <string>
#include <thread>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

#include "sensors_errors.h"
#include "vibration_priority_manager.h"

#undef LOG_TAG
#define LOG_TAG "VibrationPriorityManagerTest"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
using namespace Security::AccessToken;
using Security::AccessToken::AccessTokenID;

#define PRIORITY_MANAGER DelayedSingleton<VibrationPriorityManager>::GetInstance()

class VibrationPriorityManagerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void VibrationPriorityManagerTest::SetUpTestCase()
{
}

void VibrationPriorityManagerTest::TearDownTestCase()
{
}

void VibrationPriorityManagerTest::SetUp()
{
}

void VibrationPriorityManagerTest::TearDown()
{
}

HWTEST_F(VibrationPriorityManagerTest, InitVibrateWhenRing_001, TestSize.Level1)
{
    MISC_HILOGI("InitVibrateWhenRing_001 in");
    ASSERT_NO_FATAL_FAILURE(PRIORITY_MANAGER->InitVibrateWhenRing());
    MISC_HILOGI("InitVibrateWhenRing_001 out");
}

HWTEST_F(VibrationPriorityManagerTest, RegisterUser100ObserverTest_001, TestSize.Level1)
{
    MISC_HILOGI("RegisterUser100ObserverTest_001 in");
    ASSERT_NO_FATAL_FAILURE(PRIORITY_MANAGER->RegisterUser100Observer());
    MISC_HILOGI("RegisterUser100ObserverTest_001 out");
}

HWTEST_F(VibrationPriorityManagerTest, UnregisterUser100ObserverTest_001, TestSize.Level1)
{
    MISC_HILOGI("UnregisterUser100ObserverTest_001 in");
    ASSERT_NO_FATAL_FAILURE(PRIORITY_MANAGER->UnregisterUser100Observer());
    MISC_HILOGI("UnregisterUser100ObserverTest_001 out");
}
} // namespace Sensors
} // namespace OHOS