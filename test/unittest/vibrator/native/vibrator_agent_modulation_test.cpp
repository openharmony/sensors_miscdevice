/*
 * Copyright (c) 2025-2029 Huawei Device Co., Ltd.
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
#include <set>
#include <string>
#include <thread>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "parameters.h"
#include "parcel.h"
#include "securec.h"
#include "sensors_errors.h"
#include "token_setproc.h"
#include "vibrator_agent.h"
#include "vibrator_agent_type.h"
#include "vibrator_infos.h"
#include "vibrator_test_common.h"

#undef LOG_TAG
#define LOG_TAG "VibratorAgentSeekTest"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;

namespace {
constexpr int32_t CONT_TYPE_NO_CURVE_EVENT_IDX_9 = 9;
constexpr int32_t CONT_TYPE_NO_CURVE_EVENT_IDX_10 = 10;
constexpr int32_t CONT_TYPE_CURVE_EVENT_IDX_8 = 8;
constexpr int32_t CONT_TYPE_CURVE_EVENT_IDX_9 = 9;
constexpr int32_t CONT_TYPE_CURVE_EVENT_IDX_10 = 10;
constexpr int32_t FADE_IN_IDX_0 = 0;
constexpr int32_t FADE_IN_IDX_1 = 1;
constexpr int32_t FADE_OUT_IDX_10 = 10;
constexpr int32_t FADE_IN_FADE_OUT_DURATION = 1200;
constexpr int32_t MOD_NON_CONT_TYPE_EVENT_IDX = 0;
constexpr int32_t INVALID_FD = -1;
constexpr int32_t TRANSIENT_VIBRATION_DURATION = 48;
constexpr int32_t TIME_WAIT_FOR_OP = 2000;
constexpr int32_t TIME_WAIT_FOR_EACH_CASE = 200;
constexpr int32_t TEST_SUCCESS = 0;
constexpr int32_t TEST_FAILED = -1;
static MockHapToken* g_mock = nullptr;
uint64_t g_selfShellTokenId;
}

using namespace Security::AccessToken;
using Security::AccessToken::AccessTokenID;

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

class VibratorAgentModulationTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
private:
    static AccessTokenID tokenID_;
};

AccessTokenID VibratorAgentModulationTest::tokenID_ = 0;

void VibratorAgentModulationTest::SetUpTestCase()
{
    g_selfShellTokenId = GetSelfTokenID();
    VibratorTestCommon::SetTestEvironment(g_selfShellTokenId);
    std::vector<std::string> reqPerm;
    reqPerm.emplace_back("ohos.permission.VIBRATE");
    g_mock = new (std::nothrow) MockHapToken("vibratoragent_test", reqPerm, true);
    AccessTokenIDEx tokenIdEx = {0};
    VibratorTestCommon::AllocTestHapToken(g_infoManagerTestInfoParms, g_infoManagerTestPolicyPrams, tokenIdEx);
    tokenID_ = tokenIdEx.tokenIdExStruct.tokenID;
    VibratorTestCommon::AllocAndGrantHapTokenByTest(g_infoManagerTestInfoParms, g_infoManagerTestPolicyPrams);
}

void VibratorAgentModulationTest::TearDownTestCase()
{
    if (g_mock != nullptr) {
        delete g_mock;
        g_mock = nullptr;
    }
    EXPECT_EQ(0, SetSelfTokenID(g_selfShellTokenId));
    VibratorTestCommon::ResetTestEvironment();
}

void VibratorAgentModulationTest::SetUp()
{
}

void VibratorAgentModulationTest::TearDown()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_EACH_CASE));
}

int32_t PlayModulatedPattern(const VibratorPackage& package)
{
    VibratorIdentifier identifier = {
        .deviceId = -1,
        .vibratorId = -1,
    };
    bool isSupport = IsSupportVibratorCustomEnhanced(identifier);
    if (!isSupport) {
        MISC_HILOGI("PlayModulatedPattern: enhanced not supported");
        return TEST_SUCCESS;
    } else {
        MISC_HILOGI("PlayModulatedPattern: enhanced is supported");
    }
    if (!SetUsageEnhanced(identifier, USAGE_UNKNOWN)) {
        return TEST_FAILED;
    }
    for (int32_t idx = 0; idx < package.patternNum; idx++) {
        int32_t res = PlayPatternEnhanced(identifier, package.patterns[0]);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
        CancelEnhanced(identifier);
        if (res != TEST_SUCCESS) {
            return TEST_FAILED;
        }
    }
    return TEST_SUCCESS;
}

void PrintVibratorPackageInfo(const VibratorPackage& package, std::string functionName)
{
    MISC_HILOGI("FunctionName:%{public}s data, package patternNum:%{public}d", functionName.c_str(),
        package.patternNum);
    MISC_HILOGI("FunctionName:%{public}s data, package packageDuration:%{public}d", functionName.c_str(),
        package.packageDuration);
    for (int32_t i = 0; i < package.patternNum; i++) {
        MISC_HILOGI("FunctionName:%{public}s data, pattern time:%{public}d", functionName.c_str(),
            package.patterns[i].time);
        MISC_HILOGI("FunctionName:%{public}s data, pattern patternDuration:%{public}d", functionName.c_str(),
            package.patterns[i].patternDuration);
        MISC_HILOGI("FunctionName:%{public}s data, pattern eventNum:%{public}d", functionName.c_str(),
            package.patterns[i].eventNum);
        for (int32_t j = 0; j < package.patterns[i].eventNum; j++) {
            MISC_HILOGI("FunctionName:%{public}s data, event type:%{public}d", functionName.c_str(),
                static_cast<int32_t>(package.patterns[i].events[j].type));
            MISC_HILOGI("FunctionName:%{public}s data, event time:%{public}d", functionName.c_str(),
                package.patterns[i].events[j].time);
            MISC_HILOGI("FunctionName:%{public}s data, event duration:%{public}d", functionName.c_str(),
                package.patterns[i].events[j].duration);
            MISC_HILOGI("FunctionName:%{public}s data, event intensity:%{public}d", functionName.c_str(),
                package.patterns[i].events[j].intensity);
            MISC_HILOGI("FunctionName:%{public}s data, event frequency:%{public}d", functionName.c_str(),
                package.patterns[i].events[j].frequency);
            MISC_HILOGI("FunctionName:%{public}s data, event index:%{public}d", functionName.c_str(),
                package.patterns[i].events[j].index);
            MISC_HILOGI("FunctionName:%{public}s data, event pointNum:%{public}d", functionName.c_str(),
                package.patterns[i].events[j].pointNum);
            for (int32_t k = 0; k < package.patterns[i].events[j].pointNum; k++) {
                MISC_HILOGI("FunctionName:%{public}s data, points time:%{public}d", functionName.c_str(),
                    package.patterns[i].events[j].points[k].time);
                MISC_HILOGI("FunctionName:%{public}s data, points intensity:%{public}d", functionName.c_str(),
                    package.patterns[i].events[j].points[k].intensity);
                MISC_HILOGI("FunctionName:%{public}s data, points frequency:%{public}d", functionName.c_str(),
                    package.patterns[i].events[j].points[k].frequency);
            }
        }
    }
}

void FreeEvent(VibratorEvent& event)
{
    if (event.pointNum != 0 && event.points != nullptr) {
        free(event.points);
        event.points = nullptr;
    }
}

bool IsVibratorCurvePointIdentical(const VibratorCurvePoint& first, const VibratorCurvePoint& second)
{
    return first.frequency == second.frequency && first.intensity == second.intensity && first.time == second.time;
}

bool IsEventIdentical(const VibratorEvent& first, const VibratorEvent& second)
{
    if (first.type != second.type || first.time != second.time || first.duration != second.duration ||
        first.intensity != second.intensity || first.frequency != second.frequency || first.index != second.index ||
        first.pointNum != second.pointNum) {
        return false;
    }
    if (first.type != EVENT_TYPE_CONTINUOUS) {
        return first.pointNum == 0;
    }
    if (first.pointNum == 0) {
        return true;
    }
    const VibratorCurvePoint* firstCurve = first.points;
    const VibratorCurvePoint* secondCurve = second.points;
    if (firstCurve == nullptr || secondCurve == nullptr) {
        return false;
    }
    for (int32_t idx = 0; idx < first.pointNum; idx++) {
        if (!IsVibratorCurvePointIdentical(firstCurve[idx], secondCurve[idx])) {
            return false;
        }
    }
    return true;
}

bool IsPatternIdentical(const VibratorPattern& first, const VibratorPattern& second)
{
    if (first.time != second.time || first.eventNum != second.eventNum ||
        first.patternDuration != second.patternDuration) {
        return false;
    }
    for (int32_t idx = 0; idx < first.eventNum; idx++) {
        if (!IsEventIdentical(first.events[idx], second.events[idx])) {
            return false;
        }
    }
    return true;
}

bool IsPackageIdentical(const VibratorPackage& first, const VibratorPackage& second)
{
    if (first.patternNum != second.patternNum || first.packageDuration != second.packageDuration) {
        return false;
    }
    for (int32_t idx = 0; idx < first.patternNum; idx++) {
        if (!IsPatternIdentical(first.patterns[idx], second.patterns[idx])) {
            return false;
        }
    }
    return true;
}

int GetFdWithRealpath(const char* path)
{
    char filePath[PATH_MAX];
    char *resolvedPath = realpath(path, filePath);
    if (resolvedPath == nullptr) {
        MISC_HILOGE("GetFdWithRealpath: invalid path %{public}s", path);
        return INVALID_FD;
    }
    int fd = open(filePath, O_RDONLY);
    if (fd < 0) {
        MISC_HILOGE("GetFdWithRealpath: failed to open %{public}s", filePath);
        return INVALID_FD;
    }
    return fd;
}

bool ConvertFileToVibratorPackage(const char* functionName, const char* filePath, VibratorPackage& vibratorPackage)
{
    MISC_HILOGI("ConvertFileToVibratorPackage in: %{public}s", functionName);
    int fd = GetFdWithRealpath(filePath);
    if (fd < 0) {
        MISC_HILOGI("ConvertFileToVibratorPackage: failed to open %{public}s in: %{public}s", filePath, functionName);
        return false;
    }
    MISC_HILOGD("fd:%{public}d", fd);
    VibratorFileDescription vfd;
    struct stat64 statbuf = { 0 };
    if (fstat64(fd, &statbuf) != 0) {
        MISC_HILOGE("failed to call fstat64 with param: %{public}s", filePath);
        close(fd);
        return false;
    }
    vfd.fd = fd;
    vfd.offset = 0;
    vfd.length = statbuf.st_size;
    if (PreProcess(vfd, vibratorPackage) != 0) {
        MISC_HILOGE("failed to call PreProcess with param: %{public}s", filePath);
        close(fd);
        return false;
    }
    PrintVibratorPackageInfo(vibratorPackage, std::string(functionName));
    return true;
}

void ConvertEventToParams(VibratorEvent& event, VibratorCurvePoint** curvePointArg,
    int32_t& curvePointNum, int32_t& duration)
{
    if (curvePointArg == nullptr) {
        MISC_HILOGE("failed to ConvertEventToParams due to nullptr");
    }
    *curvePointArg = event.points;
    curvePointNum = event.pointNum;
    duration = event.duration + event.time;
    for (int32_t idx = 0; idx < curvePointNum; idx++) {
        (*curvePointArg)[idx].time += event.time;
    }
}

VibratorEvent GetExpectedEventInModulateNonContinuousType()
{
    return VibratorEvent{.type = EVENT_TYPE_TRANSIENT, .time = 0, .duration = TRANSIENT_VIBRATION_DURATION,
        .intensity = 0, .frequency = 31, .index = 0, .pointNum = 0, .points = nullptr};
}

HWTEST_F(VibratorAgentModulationTest, ModulateNonContinuousType, TestSize.Level1)
{
    MISC_HILOGI("ModulateNonContinuousType in");
    VibratorPackage package;
    VibratorPackage modulationPackage;
    VibratorPackage packageAfterModulation;
    ASSERT_TRUE(ConvertFileToVibratorPackage(
        "ModulateNonContinuousType", "/data/test/vibrator/package_before_modulation.json", package));
    ASSERT_TRUE(ConvertFileToVibratorPackage(
        "ModulateNonContinuousType", "/data/test/vibrator/modulation_curve.json", modulationPackage));
    ASSERT_TRUE(modulationPackage.patternNum >= 1);
    ASSERT_TRUE(modulationPackage.patterns[0].eventNum >= 1);
    ASSERT_NE(modulationPackage.patterns[0].events, nullptr);
    VibratorCurvePoint* modulationCurve = nullptr;
    int32_t curvePointNum = 0;
    int32_t duration = 0;
    ConvertEventToParams(modulationPackage.patterns[0].events[0], &modulationCurve, curvePointNum, duration);
    ASSERT_EQ(ModulatePackage(modulationCurve, curvePointNum, duration, package, packageAfterModulation), 0);
    ASSERT_EQ(package.patternNum, packageAfterModulation.patternNum);
    ASSERT_EQ(package.packageDuration, packageAfterModulation.packageDuration);
    const VibratorPattern& pattern = package.patterns[0];
    const VibratorPattern& patternAfterMod = packageAfterModulation.patterns[0];
    ASSERT_EQ(pattern.eventNum, patternAfterMod.eventNum);
    for (int32_t eventIdx = 0; eventIdx < pattern.eventNum; eventIdx++) {
        if (eventIdx == MOD_NON_CONT_TYPE_EVENT_IDX) {
            VibratorEvent expectedEvent = GetExpectedEventInModulateNonContinuousType();
            ASSERT_NE(expectedEvent.type, EVENT_TYPE_UNKNOWN);
            ASSERT_TRUE(IsEventIdentical(expectedEvent, patternAfterMod.events[eventIdx]));
            FreeEvent(expectedEvent);
        } else {
            ASSERT_TRUE(IsEventIdentical(pattern.events[eventIdx], patternAfterMod.events[eventIdx]));
        }
    }
    ASSERT_EQ(PlayModulatedPattern(packageAfterModulation), TEST_SUCCESS);
    ASSERT_EQ(FreeVibratorPackage(package), 0);
    ASSERT_EQ(FreeVibratorPackage(packageAfterModulation), 0);
    ASSERT_EQ(FreeVibratorPackage(modulationPackage), 0);
    MISC_HILOGI("ModulateNonContinuousType end");
}

VibratorEvent GetContinuousTypeWithCurvePointsExpectation()
{
    std::vector<VibratorCurvePoint> curveVec;
    curveVec.emplace_back(VibratorCurvePoint{.time = 0, .intensity = 0, .frequency = -10});
    curveVec.emplace_back(VibratorCurvePoint{.time = 100, .intensity = 17, .frequency = -10});
    curveVec.emplace_back(VibratorCurvePoint{.time = 200, .intensity = 50, .frequency = 10});
    curveVec.emplace_back(VibratorCurvePoint{.time = 254, .intensity = 0, .frequency = 20});
    VibratorCurvePoint *curvePoints = (VibratorCurvePoint *)calloc(curveVec.size(), sizeof(VibratorCurvePoint));
    if (curvePoints == nullptr) {
        MISC_HILOGE("failed to allocate memory for VibratorCurvePoint");
        curveVec.clear();
    } else {
        uint32_t copyBytesCount = curveVec.size() * sizeof(VibratorCurvePoint);
        memcpy_s(curvePoints, copyBytesCount, curveVec.data(), copyBytesCount);
    }
    return VibratorEvent{.type = curvePoints == nullptr ? EVENT_TYPE_UNKNOWN : EVENT_TYPE_CONTINUOUS,
        .time = 410, .duration = 254, .intensity = 38, .frequency = 30, .index = 0,
        .pointNum = (int32_t)curveVec.size(), .points = curvePoints};
}

VibratorEvent GetContinuousTypeWithoutCurvePointsExpectationForEvent(int32_t idx)
{
    switch (idx) {
        case CONT_TYPE_NO_CURVE_EVENT_IDX_9:
            return VibratorEvent{.type =  EVENT_TYPE_CONTINUOUS, .time = 940, .duration = 160,
                .intensity = 7, .frequency = 57, .index = 0, .pointNum = 0, .points = nullptr};
            break;
        case CONT_TYPE_NO_CURVE_EVENT_IDX_10:
            return VibratorEvent{.type =  EVENT_TYPE_CONTINUOUS, .time = 1100, .duration = 100,
                .intensity = 14, .frequency = 64, .index = 0, .pointNum = 0, .points = nullptr};
            break;
        default:
            return VibratorEvent{.type =  EVENT_TYPE_UNKNOWN, .time = -1, .duration = -1,
                .intensity = -1, .frequency = -1, .index = 0, .pointNum = 0, .points = nullptr};
            break;
    }
}

HWTEST_F(VibratorAgentModulationTest, ContinuousTypeWithCurvePoint, TestSize.Level1)
{
    MISC_HILOGI("ContinuousTypeWithCurvePoint in");
    VibratorPackage package;
    VibratorPackage modulationPackage;
    VibratorPackage packageAfterModulation;
    ASSERT_TRUE(ConvertFileToVibratorPackage(
        "ContinuousTypeWithCurvePoint", "/data/test/vibrator/package_before_modulation.json", package));
    ASSERT_TRUE(ConvertFileToVibratorPackage(
        "ContinuousTypeWithCurvePoint", "/data/test/vibrator/modulation_curve.json", modulationPackage));
    ASSERT_TRUE(modulationPackage.patternNum >= 1);
    ASSERT_TRUE(modulationPackage.patterns[0].eventNum >= 1);
    ASSERT_NE(modulationPackage.patterns[0].events, nullptr);
    VibratorCurvePoint* modulationCurve = nullptr;
    int32_t curvePointNum = 0;
    int32_t duration = 0;
    ConvertEventToParams(modulationPackage.patterns[0].events[1], &modulationCurve, curvePointNum, duration);
    ASSERT_EQ(ModulatePackage(modulationCurve, curvePointNum, duration, package, packageAfterModulation), 0);
    ASSERT_FALSE(IsPackageIdentical(package, packageAfterModulation));
    ASSERT_EQ(packageAfterModulation.patternNum, package.patternNum);
    ASSERT_EQ(packageAfterModulation.patterns[0].eventNum, package.patterns[0].eventNum);
    const VibratorPattern& pattern = packageAfterModulation.patterns[0];
    const VibratorPattern& originalPattern = package.patterns[0];
    for (int32_t idx = 0; idx < pattern.eventNum; idx++) {
        if (idx != 7) {
            ASSERT_TRUE(IsEventIdentical(originalPattern.events[idx], pattern.events[idx]));
        } else {
            const VibratorEvent& afterModEvent = pattern.events[idx];
            VibratorEvent expectedEvent = GetContinuousTypeWithCurvePointsExpectation();
            ASSERT_NE(expectedEvent.type, EVENT_TYPE_UNKNOWN);
            ASSERT_TRUE(IsEventIdentical(afterModEvent, expectedEvent));
            FreeEvent(expectedEvent);
        }
    }
    ASSERT_EQ(PlayModulatedPattern(packageAfterModulation), TEST_SUCCESS);
    ASSERT_EQ(FreeVibratorPackage(package), 0);
    ASSERT_EQ(FreeVibratorPackage(packageAfterModulation), 0);
    ASSERT_EQ(FreeVibratorPackage(modulationPackage), 0);
    MISC_HILOGI("ContinuousTypeWithCurvePoint end");
}

HWTEST_F(VibratorAgentModulationTest, ContinuousTypeWithoutCurvePoint, TestSize.Level1)
{
    MISC_HILOGI("ContinuousTypeWithoutCurvePoint in");
    VibratorPackage package, modulationPackage, packageAfterModulation;
    ASSERT_TRUE(ConvertFileToVibratorPackage(
        "ContinuousTypeWithoutCurvePoint", "/data/test/vibrator/package_before_modulation.json", package));
    ASSERT_TRUE(ConvertFileToVibratorPackage(
        "ContinuousTypeWithoutCurvePoint", "/data/test/vibrator/modulation_curve.json", modulationPackage));
    ASSERT_TRUE(modulationPackage.patternNum >= 1);
    ASSERT_TRUE(modulationPackage.patterns[0].eventNum >= 1);
    ASSERT_NE(modulationPackage.patterns[0].events, nullptr);
    VibratorCurvePoint* modulationCurve = nullptr;
    int32_t curvePointNum = 0;
    int32_t duration = 0;
    ConvertEventToParams(modulationPackage.patterns[0].events[2], &modulationCurve, curvePointNum, duration);
    ASSERT_EQ(ModulatePackage(modulationCurve, curvePointNum, duration, package, packageAfterModulation), 0);
    ASSERT_FALSE(IsPackageIdentical(package, packageAfterModulation));
    ASSERT_EQ(packageAfterModulation.patternNum, package.patternNum);
    ASSERT_EQ(packageAfterModulation.patterns[0].eventNum, package.patterns[0].eventNum);
    const VibratorPattern& pattern = packageAfterModulation.patterns[0];
    const VibratorPattern& originalPattern = package.patterns[0];
    for (int32_t idx = 0; idx < pattern.eventNum; idx++) {
        if (idx != CONT_TYPE_NO_CURVE_EVENT_IDX_9 && idx != CONT_TYPE_NO_CURVE_EVENT_IDX_10) {
            ASSERT_TRUE(IsEventIdentical(originalPattern.events[idx], pattern.events[idx]));
        } else {
            const VibratorEvent& event = pattern.events[idx];
            VibratorEvent expectedEvent = GetContinuousTypeWithoutCurvePointsExpectationForEvent(idx);
            ASSERT_TRUE(IsEventIdentical(event, expectedEvent));
            FreeEvent(expectedEvent);
        }
    }
    ASSERT_EQ(PlayModulatedPattern(packageAfterModulation), TEST_SUCCESS);
    ASSERT_EQ(FreeVibratorPackage(package), 0);
    ASSERT_EQ(FreeVibratorPackage(packageAfterModulation), 0);
    ASSERT_EQ(FreeVibratorPackage(modulationPackage), 0);
    MISC_HILOGI("ContinuousTypeWithoutCurvePoint end");
}

VibratorEvent GetExpectedEventForContinuousTypeWithCurvePointAndEventStartTime(int idx)
{
    VibratorCurvePoint* curvePoints = nullptr;
    std::vector<VibratorCurvePoint> pointsVec;
    switch (idx) {
        case CONT_TYPE_CURVE_EVENT_IDX_8:
            pointsVec.emplace_back(VibratorCurvePoint{.time = 0, .intensity = 0, .frequency = 0});
            pointsVec.emplace_back(VibratorCurvePoint{.time = 100, .intensity = 100, .frequency = 0});
            pointsVec.emplace_back(VibratorCurvePoint{.time = 200, .intensity = 17, .frequency = -10});
            pointsVec.emplace_back(VibratorCurvePoint{.time = 254, .intensity = 0, .frequency = -10});
            curvePoints = (VibratorCurvePoint *)calloc(pointsVec.size(), sizeof(VibratorCurvePoint));
            if (curvePoints == nullptr) {
                MISC_HILOGE("failed to allocate memory for VibratorCurvePoint");
            } else {
                uint32_t copyBytesCount = pointsVec.size() * sizeof(VibratorCurvePoint);
                memcpy_s(curvePoints, copyBytesCount, pointsVec.data(), copyBytesCount);
            }
            return VibratorEvent{.type =  EVENT_TYPE_CONTINUOUS, .time = 670, .duration = 254,
                .intensity = 38, .frequency = 30, .index = 0,
                .pointNum = curvePoints == nullptr ? 0 : (int32_t)pointsVec.size(), .points = curvePoints};
            break;
        case CONT_TYPE_CURVE_EVENT_IDX_9:
            return VibratorEvent{.type =  EVENT_TYPE_CONTINUOUS, .time = 940, .duration = 160,
                .intensity = 12, .frequency = 27, .index = 0, .pointNum = 0, .points = nullptr};
            break;
        case CONT_TYPE_CURVE_EVENT_IDX_10:
            return VibratorEvent{.type =  EVENT_TYPE_CONTINUOUS, .time = 1100, .duration = 100,
                .intensity = 18, .frequency = 64, .index = 0, .pointNum = 0, .points = nullptr};
            break;
        default:
            return VibratorEvent{.type = EVENT_TYPE_UNKNOWN, .time = -1, .duration = -1, .intensity = -1,
                .frequency = -1, .index = 0, .pointNum = 0, .points = nullptr};
            break;
    }
}

HWTEST_F(VibratorAgentModulationTest, ContinuousTypeWithCurvePointAndEventStartTime, TestSize.Level1)
{
    MISC_HILOGI("ContinuousTypeWithCurvePointAndEventStartTime in");
    VibratorPackage package;
    VibratorPackage modulationPackage;
    VibratorPackage packageAfterModulation;
    ASSERT_TRUE(ConvertFileToVibratorPackage("ContinuousTypeWithCurvePointAndEventStartTime",
        "/data/test/vibrator/package_before_modulation.json", package));
    ASSERT_TRUE(ConvertFileToVibratorPackage("ContinuousTypeWithCurvePointAndEventStartTime",
        "/data/test/vibrator/modulation_curve.json", modulationPackage));
    ASSERT_TRUE(modulationPackage.patternNum >= 1);
    ASSERT_TRUE(modulationPackage.patterns[0].eventNum >= 1);
    ASSERT_NE(modulationPackage.patterns[0].events, nullptr);
    VibratorEvent& modulationEvent = modulationPackage.patterns[0].events[1];
    modulationEvent.duration = 270;
    modulationEvent.time = 850;
    VibratorCurvePoint* modulationCurve = nullptr;
    int32_t curvePointNum = 0;
    int32_t duration = 0;
    ConvertEventToParams(modulationEvent, &modulationCurve, curvePointNum, duration);
    ASSERT_EQ(ModulatePackage(modulationCurve, curvePointNum, duration, package, packageAfterModulation), 0);
    ASSERT_FALSE(IsPackageIdentical(package, packageAfterModulation));
    ASSERT_EQ(packageAfterModulation.patternNum, package.patternNum);
    ASSERT_EQ(packageAfterModulation.patterns[0].eventNum, package.patterns[0].eventNum);
    const VibratorPattern& pattern = packageAfterModulation.patterns[0];
    const VibratorPattern& originalPattern = package.patterns[0];
    for (int32_t idx = 0; idx < pattern.eventNum; idx++) {
        if (idx < 8) {
            ASSERT_TRUE(IsEventIdentical(originalPattern.events[idx], pattern.events[idx]));
        } else {
            const VibratorEvent& event = pattern.events[idx];
            VibratorEvent expectedEvent = GetExpectedEventForContinuousTypeWithCurvePointAndEventStartTime(idx);
            ASSERT_NE(expectedEvent.type, EVENT_TYPE_UNKNOWN);
            ASSERT_TRUE(IsEventIdentical(event, expectedEvent));
            FreeEvent(expectedEvent);
        }
    }
    ASSERT_EQ(PlayModulatedPattern(packageAfterModulation), TEST_SUCCESS);
    ASSERT_EQ(FreeVibratorPackage(package), 0);
    ASSERT_EQ(FreeVibratorPackage(packageAfterModulation), 0);
    ASSERT_EQ(FreeVibratorPackage(modulationPackage), 0);
    MISC_HILOGI("ContinuousTypeWithCurvePointAndEventStartTime end");
}

HWTEST_F(VibratorAgentModulationTest, InvalidModulationEvent, TestSize.Level1)
{
    MISC_HILOGI("InvalidModulationEvent in");
    VibratorPackage package;
    VibratorPackage packageAfterModulation;
    ASSERT_TRUE(ConvertFileToVibratorPackage("InvalidModulationEvent",
        "/data/test/vibrator/package_before_modulation.json", package));
    ASSERT_NE(ModulatePackage(nullptr, -1, 220, package, packageAfterModulation), 0);
    ASSERT_EQ(FreeVibratorPackage(package), 0);
    MISC_HILOGI("InvalidModulationEvent end");
}

HWTEST_F(VibratorAgentModulationTest, EventWithInvalidAmountOfPoints, TestSize.Level1)
{
    MISC_HILOGI("EventWithInvalidAmountOfPoints in");
    VibratorPackage package;
    VibratorPackage modulationPackage;
    VibratorPackage packageAfterModulation;
    ASSERT_TRUE(ConvertFileToVibratorPackage("ContinuousTypeWithCurvePointAndEventStartTime",
        "/data/test/vibrator/package_before_modulation.json", package));
    ASSERT_TRUE(ConvertFileToVibratorPackage("ContinuousTypeWithCurvePointAndEventStartTime",
        "/data/test/vibrator/modulation_curve.json", modulationPackage));
    VibratorEvent& modulationEvent = modulationPackage.patterns[0].events[1];
    VibratorCurvePoint* modulationCurve = nullptr;
    int32_t curvePointNum = 0;
    int32_t duration = 0;
    package.patterns[0].events[8].pointNum = 2;
    ConvertEventToParams(modulationEvent, &modulationCurve, curvePointNum, duration);
    ASSERT_NE(ModulatePackage(modulationCurve, curvePointNum, duration, package, packageAfterModulation), 0);
    ASSERT_EQ(FreeVibratorPackage(package), 0);
    ASSERT_EQ(FreeVibratorPackage(modulationPackage), 0);
    MISC_HILOGI("EventWithInvalidAmountOfPoints end");
}

bool GenerateFadeInFadeOutCurve(VibratorCurvePoint** modulationCurve, int32_t& curvePointNum, int32_t& duration)
{
    if (modulationCurve == nullptr) {
        MISC_HILOGI("Invalid pointer for modulationCurve");
        return false;
    }
    std::vector<VibratorCurvePoint> curveVec {
        VibratorCurvePoint{.time = 0, .intensity = 0, .frequency = 0},
        VibratorCurvePoint{.time = 20, .intensity = 30, .frequency = 0},
        VibratorCurvePoint{.time = 40, .intensity = 50, .frequency = 0},
        VibratorCurvePoint{.time = 70, .intensity = 70, .frequency = 0},
        VibratorCurvePoint{.time = 90, .intensity = 100, .frequency = 0},
        VibratorCurvePoint{.time = 1000, .intensity = 90, .frequency = 0},
        VibratorCurvePoint{.time = 1120, .intensity = 70, .frequency = 0},
        VibratorCurvePoint{.time = 1150, .intensity = 50, .frequency = 0},
        VibratorCurvePoint{.time = 1170, .intensity = 30, .frequency = 0},
        VibratorCurvePoint{.time = 1200, .intensity = 0, .frequency = 0}
    };
    VibratorCurvePoint* curve = (VibratorCurvePoint*)malloc(curveVec.size() * sizeof(VibratorCurvePoint));
    if (curve == nullptr) {
        MISC_HILOGI("generateFadeInFadeOutCurve: failed to allocate memory for curve");
        return false;
    }
    std::copy(curveVec.begin(), curveVec.end(), curve);
    *modulationCurve = curve;
    curvePointNum = (int32_t)curveVec.size();
    duration = FADE_IN_FADE_OUT_DURATION;
    return true;
}

VibratorEvent GetExpectedOutputEventsForFadeInFadeOut(int32_t idx)
{
    VibratorCurvePoint* curvePoints = nullptr;
    std::vector<VibratorCurvePoint> curveVec;
    switch (idx) {
        case FADE_IN_IDX_0:
            return VibratorEvent{.type =  EVENT_TYPE_TRANSIENT, .time = 0, .duration = TRANSIENT_VIBRATION_DURATION,
                .intensity = 0, .frequency = 31, .index = 0, .pointNum = 0, .points = nullptr};
            break;
        case FADE_IN_IDX_1:
            curveVec.emplace_back(VibratorCurvePoint{.time = 0, .intensity = 0, .frequency = 0});
            curveVec.emplace_back(VibratorCurvePoint{.time = 1, .intensity = 50, .frequency = 0});
            curveVec.emplace_back(VibratorCurvePoint{.time = 40, .intensity = 70, .frequency = 0});
            curveVec.emplace_back(VibratorCurvePoint{.time = 54, .intensity = 0, .frequency = 0});
            curvePoints = (VibratorCurvePoint *)calloc(curveVec.size(), sizeof(VibratorCurvePoint));
            if (curvePoints == nullptr) {
                MISC_HILOGE("failed to allocate memory for VibratorCurvePoint");
                curveVec.clear();
            } else {
                uint32_t copyBytesCount = curveVec.size() * sizeof(VibratorCurvePoint);
                memcpy_s(curvePoints, copyBytesCount, curveVec.data(), copyBytesCount);
            }
            return VibratorEvent{.type =  EVENT_TYPE_CONTINUOUS, .time = 40, .duration = 54,
                .intensity = 38, .frequency = 30, .index = 0, .pointNum = (int32_t)curveVec.size(),
                .points = curvePoints};
            break;
        case FADE_OUT_IDX_10:
            return VibratorEvent{.type =  EVENT_TYPE_CONTINUOUS, .time = 1100, .duration = 100,
                .intensity = 21, .frequency = 44, .index = 0, .pointNum = 0, .points = nullptr};
            break;
        default:
            return VibratorEvent{.type = EVENT_TYPE_UNKNOWN, .time = -1, .duration = -1, .intensity = -1,
                .frequency = -1, .index = 0, .pointNum = 0, .points = nullptr};
            break;
    }
}

HWTEST_F(VibratorAgentModulationTest, FadeInFadeOut, TestSize.Level1)
{
    MISC_HILOGI("FadeInFadeOut in");
    VibratorCurvePoint* modulationCurve;
    int32_t curvePointNum;
    int32_t duration;
    ASSERT_TRUE(GenerateFadeInFadeOutCurve(&modulationCurve, curvePointNum, duration));
    VibratorPackage package;
    VibratorPackage packageAfterModulation;
    ASSERT_TRUE(ConvertFileToVibratorPackage("FadeInFadeOut",
        "/data/test/vibrator/package_before_modulation.json", package));
    ASSERT_EQ(ModulatePackage(modulationCurve, curvePointNum, duration, package, packageAfterModulation), 0);
    const VibratorPattern& originalPattern = package.patterns[0];
    const VibratorPattern& afterModPattern = packageAfterModulation.patterns[0];
    std::set<int32_t> modifiedEventIdx = {FADE_IN_IDX_0, FADE_IN_IDX_1, FADE_OUT_IDX_10};
    std::set<VibratorCurvePoint> expectedOuput;
    for (int32_t eventIdx = 0; eventIdx < originalPattern.eventNum; eventIdx++) {
        if (modifiedEventIdx.find(eventIdx) == modifiedEventIdx.end()) {
            ASSERT_TRUE(IsEventIdentical(originalPattern.events[eventIdx], afterModPattern.events[eventIdx]));
        } else {
            const VibratorEvent& afterModEvent = afterModPattern.events[eventIdx];
            VibratorEvent expectedEvent = GetExpectedOutputEventsForFadeInFadeOut(eventIdx);
            ASSERT_NE(expectedEvent.type, EVENT_TYPE_UNKNOWN);
            ASSERT_TRUE(IsEventIdentical(afterModEvent, expectedEvent));
            FreeEvent(expectedEvent);
        }
    }
    ASSERT_EQ(PlayModulatedPattern(packageAfterModulation), TEST_SUCCESS);
    ASSERT_EQ(FreeVibratorPackage(package), 0);
    ASSERT_EQ(FreeVibratorPackage(packageAfterModulation), 0);
    free(modulationCurve);
    modulationCurve = nullptr;
    MISC_HILOGI("FadeInFadeOut end");
}

} // namespace Sensors
} // namespace OHOS
