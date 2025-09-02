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

#include "system_ability_definition.h"

#include "miscdevice_service.h"
#include "sensors_errors.h"

#undef LOG_TAG
#define LOG_TAG "VibratorServiceTest"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;

namespace {
auto g_miscdeviceService = MiscdeviceDelayedSpSingleton<MiscdeviceService>::GetInstance();
}

class VibratorServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void VibratorServiceTest::SetUpTestCase()
{
}

void VibratorServiceTest::TearDownTestCase()
{
}

void VibratorServiceTest::SetUp()
{
}

void VibratorServiceTest::TearDown()
{
}

HWTEST_F(VibratorServiceTest, OnAddSystemAbilityTest_001, TestSize.Level1)
{
    MISC_HILOGI("OnAddSystemAbilityTest_001 in");
    std::string deviceId = "0";
    ASSERT_NE(g_miscdeviceService, nullptr);
    g_miscdeviceService->isVibrationPriorityReady_ = true;
    EXPECT_NO_FATAL_FAILURE(g_miscdeviceService->OnAddSystemAbility(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID, deviceId));
}
} // namespace Sensors
} // namespace OHOS