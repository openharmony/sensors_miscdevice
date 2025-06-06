/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "parameters.h"
#include "sensors_errors.h"
#include "vibrator_agent.h"
#include "vibrator_agent_type.h"

#undef LOG_TAG
#define LOG_TAG "VibratorAgentSeekTest"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;

namespace {
constexpr int32_t TIME_WAIT_FOR_OP_TWO_HUNDRED = 200;
constexpr int32_t CONT_TYPE_NO_CURVE_EVENT_IDX_9 = 9;
constexpr int32_t CONT_TYPE_NO_CURVE_EVENT_IDX_10 = 10;
constexpr int32_t CONT_TYPE_CURVE_EVENT_IDX_8 = 8;
constexpr int32_t CONT_TYPE_CURVE_EVENT_IDX_9 = 9;
constexpr int32_t CONT_TYPE_CURVE_EVENT_IDX_10 = 10;
constexpr int32_t CURVE_FREQUENCY_NO_MODIFICATION = 0;
constexpr int32_t CURVE_INTENSITY_NO_MODIFICATION = 100;
constexpr int32_t CURVE_TIME_NO_MODIFICATION = 0;
}

class VibratorAgentModulationTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
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

void VibratorAgentModulationTest::SetUpTestCase()
{
}

void VibratorAgentModulationTest::TearDownTestCase()
{
}

void VibratorAgentModulationTest::SetUp()
{
}

void VibratorAgentModulationTest::TearDown()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_TWO_HUNDRED));
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

bool IsVibratorCurvePointIdentical(const VibratorCurvePoint& first, const VibratorCurvePoint& second)
{
    return first.frequency == second.frequency && first.intensity == second.intensity && first.time == second.time;
}

bool IsEquivalentEvent(const VibratorEvent& first, const VibratorEvent& second)
{
    if (first.time != second.time || first.duration != second.duration || first.intensity != second.intensity ||
        first.frequency != second.frequency || first.index != second.index || first.type != second.type) {
        return false;
    }
    if ((first.pointNum == 0 && second.pointNum == 1) || (first.pointNum == 1 && second.pointNum == 0)) {
        const VibratorCurvePoint& onlyPoint = first.pointNum == 1 ? first.points[0] : second.points[0];
        return onlyPoint.frequency == CURVE_FREQUENCY_NO_MODIFICATION &&
            onlyPoint.intensity == CURVE_INTENSITY_NO_MODIFICATION &&
            onlyPoint.time == CURVE_TIME_NO_MODIFICATION;
    }
    return false;
}

bool IsEventIdentical(const VibratorEvent& first, const VibratorEvent& second)
{
    if (first.time != second.time || first.duration != second.duration || first.intensity != second.intensity ||
        first.frequency != second.frequency || first.index != second.index || first.type != second.type) {
        return false;
    }
    if (first.type != EVENT_TYPE_CONTINUOUS) {
        if (first.pointNum != 0 || second.pointNum != 0) {
            return false;
        }
        return true;
    }
    if (IsEquivalentEvent(first, second)) {
        return true;
    }
    for (int32_t idx = 0; idx < std::min(first.pointNum, second.pointNum); idx++) {
        if (!IsVibratorCurvePointIdentical(first.points[idx], second.points[idx])) {
            return false;
        }
    }
    if (first.pointNum == second.pointNum) {
        return true;
    }
    const VibratorCurvePoint* points = first.pointNum > second.pointNum ? first.points : second.points;
    const int32_t duration = first.pointNum > second.pointNum ? first.duration : second.duration;
    for (int32_t idx = std::min(first.pointNum, second.pointNum);
        idx < std::max(first.pointNum, second.pointNum); idx++) {
        if (points[idx].time < duration) {
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

bool ConvertFileToVibratorPackage(const char* functionName, const char* filePath, VibratorPackage& vibratorPackage)
{
    MISC_HILOGI("ConvertFileToVibratorPackage in: %{public}s", functionName);
    int fd = open(filePath, O_RDONLY);
    if (fd < 0) {
        MISC_HILOGI("ConvertFileToVibratorPackage: failed to open %{public}s in: %{public}s", filePath, functionName);
        return false;
    }
    MISC_HILOGD("fd:%{public}d", fd);
    VibratorFileDescription vfd;
    struct stat64 statbuf = { 0 };
    if (fstat64(fd, &statbuf) != 0) {
        MISC_HILOGE("failed to call fstat64 with param: %{public}s", filePath);
        return false;
    }
    vfd.fd = fd;
    vfd.offset = 0;
    vfd.length = statbuf.st_size;
    if (PreProcess(vfd, vibratorPackage) != 0) {
        MISC_HILOGE("failed to call PreProcess with param: %{public}s", filePath);
        return false;
    }
    PrintVibratorPackageInfo(vibratorPackage, std::string(functionName));
    return true;
}

void ConvertEventToParams(VibratorEvent& event, VibratorCurvePoint*& curvePoint,
    int32_t& curvePointNum, int32_t& duration)
{
    curvePoint = event.points;
    curvePointNum = event.pointNum;
    duration = event.duration + event.time;
    for (int32_t idx = 0; idx < curvePointNum; idx++) {
        curvePoint[idx].time += event.time;
    }
}

HWTEST_F(VibratorAgentModulationTest, NonContinuousTypeIsIntact, TestSize.Level1)
{
    MISC_HILOGI("NonContinuousTypeIsIntact in");
    VibratorPackage package, modulationPackage, packageAfterModulation;
    ASSERT_TRUE(ConvertFileToVibratorPackage(
        "NonContinuousTypeIsIntact", "/data/test/vibrator/package_before_modulation.json", package));
    ASSERT_TRUE(ConvertFileToVibratorPackage(
        "NonContinuousTypeIsIntact", "/data/test/vibrator/modulation_curve.json", modulationPackage));
    ASSERT_TRUE(modulationPackage.patternNum >= 1);
    ASSERT_TRUE(modulationPackage.patterns[0].eventNum >= 1);
    ASSERT_NE(modulationPackage.patterns[0].events, nullptr);
    VibratorCurvePoint* modulationCurve = nullptr;
    int32_t curvePointNum = 0;
    int32_t duration = 0;
    ConvertEventToParams(modulationPackage.patterns[0].events[0], modulationCurve, curvePointNum, duration);
    int32_t ret = ModulatePackage(modulationCurve, curvePointNum, duration, package, packageAfterModulation);
    ASSERT_EQ(ret, 0);
    ASSERT_TRUE(IsPackageIdentical(package, packageAfterModulation));
    ret = FreeVibratorPackage(package);
    ASSERT_EQ(ret, 0);
    ret = FreeVibratorPackage(packageAfterModulation);
    ASSERT_EQ(ret, 0);
    ret = FreeVibratorPackage(modulationPackage);
    ASSERT_EQ(ret, 0);
    MISC_HILOGI("NonContinuousTypeIsIntact end");
}

void getContinuousTypeWithCurvePointsExpectation(std::vector<VibratorCurvePoint>& ans)
{
    ans.clear();
    ans.emplace_back(VibratorCurvePoint{.time = 0, .intensity = 0, .frequency = -10});
    ans.emplace_back(VibratorCurvePoint{.time = 100, .intensity = 100, .frequency = 10});
    ans.emplace_back(VibratorCurvePoint{.time = 150, .intensity = 50, .frequency = 10});
    ans.emplace_back(VibratorCurvePoint{.time = 200, .intensity = 50, .frequency = 10});
    ans.emplace_back(VibratorCurvePoint{.time = 210, .intensity = 89, .frequency = -20});
    ans.emplace_back(VibratorCurvePoint{.time = 234, .intensity = 77, .frequency = 20});
}

HWTEST_F(VibratorAgentModulationTest, ContinuousTypeWithCurvePoint, TestSize.Level1)
{
    MISC_HILOGI("ContinuousTypeWithCurvePoint in");
    VibratorPackage package, modulationPackage, packageAfterModulation;
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
    ConvertEventToParams(modulationPackage.patterns[0].events[1], modulationCurve, curvePointNum, duration);
    int32_t ret = ModulatePackage(modulationCurve, curvePointNum, duration, package, packageAfterModulation);
    ASSERT_EQ(ret, 0);
    ASSERT_FALSE(IsPackageIdentical(package, packageAfterModulation));
    ASSERT_EQ(packageAfterModulation.patternNum, package.patternNum);
    ASSERT_EQ(packageAfterModulation.patterns[0].eventNum, package.patterns[0].eventNum);
    const VibratorPattern& pattern = packageAfterModulation.patterns[0];
    const VibratorPattern& originalPattern = package.patterns[0];
    for (int32_t idx = 0; idx < pattern.eventNum; idx++) {
        if (idx != 7) {
            ASSERT_TRUE(IsEventIdentical(originalPattern.events[idx], pattern.events[idx]));
        } else {
            const VibratorEvent& event = pattern.events[idx];
            const VibratorEvent& originalEvent = originalPattern.events[idx];
            ASSERT_EQ(event.type, originalEvent.type);
            ASSERT_EQ(event.time, originalEvent.time);
            ASSERT_EQ(event.intensity, originalEvent.intensity);
            ASSERT_EQ(event.frequency, originalEvent.frequency);
            ASSERT_EQ(event.index, originalEvent.index);
            ASSERT_EQ(event.pointNum, 6);
            std::vector<VibratorCurvePoint> expectedAns;
            getContinuousTypeWithCurvePointsExpectation(expectedAns);
            for (int32_t pointIdx = 0; pointIdx < 6; pointIdx++) {
                ASSERT_TRUE(IsVibratorCurvePointIdentical(event.points[pointIdx], expectedAns[pointIdx]));
            }
        }
    }
    ret = FreeVibratorPackage(package);
    ASSERT_EQ(ret, 0);
    ret = FreeVibratorPackage(packageAfterModulation);
    ASSERT_EQ(ret, 0);
    ret = FreeVibratorPackage(modulationPackage);
    ASSERT_EQ(ret, 0);
    MISC_HILOGI("ContinuousTypeWithCurvePoint end");
}

void getContinuousTypeWithoutCurvePointsExpectationForEvent(int32_t idx, std::vector<VibratorCurvePoint>& ans)
{
    ans.clear();
    switch (idx) {
        case CONT_TYPE_NO_CURVE_EVENT_IDX_9:
            ans.emplace_back(VibratorCurvePoint{.time = 0, .intensity = 10, .frequency = 20});
            ans.emplace_back(VibratorCurvePoint{.time = 20, .intensity = 20, .frequency = -30});
            ans.emplace_back(VibratorCurvePoint{.time = 80, .intensity = 30, .frequency = 40});
            ans.emplace_back(VibratorCurvePoint{.time = 100, .intensity = 40, .frequency = -50});
            ans.emplace_back(VibratorCurvePoint{.time = 150, .intensity = 60, .frequency = 20});
            break;
        case CONT_TYPE_NO_CURVE_EVENT_IDX_10:
            ans.emplace_back(VibratorCurvePoint{.time = 0, .intensity = 60, .frequency = 20});
            ans.emplace_back(VibratorCurvePoint{.time = 40, .intensity = 70, .frequency = -30});
            ans.emplace_back(VibratorCurvePoint{.time = 80, .intensity = 80, .frequency = 40});
            break;
        default:
            break;
    }
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
    ConvertEventToParams(modulationPackage.patterns[0].events[2], modulationCurve, curvePointNum, duration);
    int32_t ret = ModulatePackage(modulationCurve, curvePointNum, duration, package, packageAfterModulation);
    ASSERT_EQ(ret, 0);
    ASSERT_FALSE(IsPackageIdentical(package, packageAfterModulation));
    ASSERT_EQ(packageAfterModulation.patternNum, package.patternNum);
    ASSERT_EQ(packageAfterModulation.patterns[0].eventNum, package.patterns[0].eventNum);
    const VibratorPattern& pattern = packageAfterModulation.patterns[0];
    const VibratorPattern& originalPattern = package.patterns[0];
    for (int32_t idx = 0; idx < pattern.eventNum; idx++) {
        if (idx != 9 && idx != 10) {
            ASSERT_TRUE(IsEventIdentical(originalPattern.events[idx], pattern.events[idx]));
            continue;
        }
        const VibratorEvent& event = pattern.events[idx];
        const VibratorEvent& originalEvent = originalPattern.events[idx];
        ASSERT_EQ(event.type, originalEvent.type);
        ASSERT_EQ(event.time, originalEvent.time);
        ASSERT_EQ(event.intensity, originalEvent.intensity);
        ASSERT_EQ(event.frequency, originalEvent.frequency);
        ASSERT_EQ(event.index, originalEvent.index);
        std::vector<VibratorCurvePoint> expectedAns;
        getContinuousTypeWithoutCurvePointsExpectationForEvent(idx, expectedAns);
        for (int32_t pointIdx = 0; pointIdx < (int32_t)expectedAns.size(); pointIdx++) {
            ASSERT_TRUE(IsVibratorCurvePointIdentical(event.points[pointIdx], expectedAns[pointIdx]));
        }
    }
    ret = FreeVibratorPackage(package);
    ASSERT_EQ(ret, 0);
    ret = FreeVibratorPackage(packageAfterModulation);
    ASSERT_EQ(ret, 0);
    ret = FreeVibratorPackage(modulationPackage);
    ASSERT_EQ(ret, 0);
    MISC_HILOGI("ContinuousTypeWithoutCurvePoint end");
}

void getExpectedCruvePoints(int idx, std::vector<VibratorCurvePoint>& ans)
{
    ans.clear();
    switch (idx) {
        case CONT_TYPE_CURVE_EVENT_IDX_8:
            ans.emplace_back(VibratorCurvePoint{.time = 0, .intensity = 0, .frequency = 0});
            ans.emplace_back(VibratorCurvePoint{.time = 100, .intensity = 100, .frequency = 0});
            ans.emplace_back(VibratorCurvePoint{.time = 200, .intensity = 100, .frequency = 0});
            ans.emplace_back(VibratorCurvePoint{.time = 230, .intensity = 17, .frequency = -10});
            break;
        case CONT_TYPE_CURVE_EVENT_IDX_9:
            ans.emplace_back(VibratorCurvePoint{.time = 0, .intensity = 17, .frequency = -10});
            ans.emplace_back(VibratorCurvePoint{.time = 60, .intensity = 100, .frequency = 10});
            ans.emplace_back(VibratorCurvePoint{.time = 110, .intensity = 50, .frequency = 10});
            break;
        case CONT_TYPE_CURVE_EVENT_IDX_10:
            ans.emplace_back(VibratorCurvePoint{.time = 0, .intensity = 50, .frequency = 10});
            ans.emplace_back(VibratorCurvePoint{.time = 10, .intensity = 89, .frequency = -20});
            ans.emplace_back(VibratorCurvePoint{.time = 20, .intensity = 100, .frequency = 0});
            break;
        default:
            break;
    }
}

HWTEST_F(VibratorAgentModulationTest, ContinuousTypeWithCurvePointAndEventStartTime, TestSize.Level1)
{
    MISC_HILOGI("ContinuousTypeWithCurvePointAndEventStartTime in");
    VibratorPackage package, modulationPackage, packageAfterModulation;
    ASSERT_TRUE(ConvertFileToVibratorPackage("ContinuousTypeWithCurvePointAndEventStartTime",
        "/data/test/vibrator/package_before_modulation.json", package));
    ASSERT_TRUE(ConvertFileToVibratorPackage("ContinuousTypeWithCurvePointAndEventStartTime",
        "/data/test/vibrator/modulation_curve.json", modulationPackage));
    ASSERT_TRUE(modulationPackage.patternNum >= 1);
    ASSERT_TRUE(modulationPackage.patterns[0].eventNum >= 1);
    ASSERT_NE(modulationPackage.patterns[0].events, nullptr);
    VibratorEvent& modulationEvent = modulationPackage.patterns[0].events[1];
    modulationEvent.duration = 220;
    modulationEvent.time = 900;
    VibratorCurvePoint* modulationCurve = nullptr;
    int32_t curvePointNum = 0;
    int32_t duration = 0;
    ConvertEventToParams(modulationEvent, modulationCurve, curvePointNum, duration);
    int32_t ret = ModulatePackage(modulationCurve, curvePointNum, duration, package, packageAfterModulation);
    ASSERT_EQ(ret, 0);
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
            const VibratorEvent& originalEvent = originalPattern.events[idx];
            ASSERT_EQ(event.type, originalEvent.type);
            ASSERT_EQ(event.time, originalEvent.time);
            std::vector<VibratorCurvePoint> expectedAns;
            getExpectedCruvePoints(idx, expectedAns);
            ASSERT_EQ(event.pointNum, (int32_t)expectedAns.size());
            for (int32_t pointIdx = 0; pointIdx < event.pointNum; pointIdx++) {
                ASSERT_TRUE(IsVibratorCurvePointIdentical(event.points[pointIdx], expectedAns[pointIdx]));
            }
        }
    }
    ret = FreeVibratorPackage(package);
    ASSERT_EQ(ret, 0);
    ret = FreeVibratorPackage(packageAfterModulation);
    ASSERT_EQ(ret, 0);
    ret = FreeVibratorPackage(modulationPackage);
    ASSERT_EQ(ret, 0);
    MISC_HILOGI("ContinuousTypeWithCurvePointAndEventStartTime end");
}

HWTEST_F(VibratorAgentModulationTest, InvalidModulationEvent, TestSize.Level1)
{
    MISC_HILOGI("InvalidModulationEvent in");
    VibratorPackage package, packageAfterModulation;
    ASSERT_TRUE(ConvertFileToVibratorPackage("InvalidModulationEvent",
        "/data/test/vibrator/package_before_modulation.json", package));
    int32_t ret = ModulatePackage(nullptr, -1, 220, package, packageAfterModulation);
    ASSERT_NE(ret, 0);
    ret = FreeVibratorPackage(package);
    ASSERT_EQ(ret, 0);
    MISC_HILOGI("InvalidModulationEvent end");
}


} // namespace Sensors
} // namespace OHOS
