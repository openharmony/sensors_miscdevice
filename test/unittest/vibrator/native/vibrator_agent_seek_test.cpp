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

#undef LOG_TAG
#define LOG_TAG "VibratorAgentSeekTest"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;

namespace {
constexpr int32_t TIME_WAIT_FOR_OP_TWO_HUNDRED = 200;
}

class VibratorAgentSeekTest : public testing::Test {
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

void VibratorAgentSeekTest::SetUpTestCase()
{
}

void VibratorAgentSeekTest::TearDownTestCase()
{
}

void VibratorAgentSeekTest::SetUp()
{
}

void VibratorAgentSeekTest::TearDown()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_TWO_HUNDRED));
}

void PrintVibratorPackageInfo(const VibratorPackage package, std::string functionName)
{
    MISC_HILOGD("FunctionName:%{public}s data, package patternNum:%{public}d", functionName.c_str(),
        package.patternNum);
    MISC_HILOGD("FunctionName:%{public}s data, package packageDuration:%{public}d", functionName.c_str(),
        package.packageDuration);
    for (int32_t i = 0; i < package.patternNum; i++) {
        MISC_HILOGD("FunctionName:%{public}s data, pattern time:%{public}d", functionName.c_str(),
            package.patterns[i].time);
        MISC_HILOGD("FunctionName:%{public}s data, pattern patternDuration:%{public}d", functionName.c_str(),
            package.patterns[i].patternDuration);
        MISC_HILOGD("FunctionName:%{public}s data, pattern eventNum:%{public}d", functionName.c_str(),
            package.patterns[i].eventNum);
        for (int32_t j = 0; j < package.patterns[i].eventNum; j++) {
            MISC_HILOGD("FunctionName:%{public}s data, event type:%{public}d", functionName.c_str(),
                static_cast<int32_t>(package.patterns[i].events[j].type));
            MISC_HILOGD("FunctionName:%{public}s data, event time:%{public}d", functionName.c_str(),
                package.patterns[i].events[j].time);
            MISC_HILOGD("FunctionName:%{public}s data, event duration:%{public}d", functionName.c_str(),
                package.patterns[i].events[j].duration);
            MISC_HILOGD("FunctionName:%{public}s data, event intensity:%{public}d", functionName.c_str(),
                package.patterns[i].events[j].intensity);
            MISC_HILOGD("FunctionName:%{public}s data, event frequency:%{public}d", functionName.c_str(),
                package.patterns[i].events[j].frequency);
            MISC_HILOGD("FunctionName:%{public}s data, event index:%{public}d", functionName.c_str(),
                package.patterns[i].events[j].index);
            MISC_HILOGD("FunctionName:%{public}s data, event pointNum:%{public}d", functionName.c_str(),
                package.patterns[i].events[j].pointNum);
            for (int32_t k = 0; k < package.patterns[i].events[j].pointNum; k++) {
                MISC_HILOGD("FunctionName:%{public}s data, points time:%{public}d", functionName.c_str(),
                    package.patterns[i].events[j].points[k].time);
                MISC_HILOGD("FunctionName:%{public}s data, points intensity:%{public}d", functionName.c_str(),
                    package.patterns[i].events[j].points[k].intensity);
                MISC_HILOGD("FunctionName:%{public}s data, points frequency:%{public}d", functionName.c_str(),
                    package.patterns[i].events[j].points[k].frequency);
            }
        }
    }
}

HWTEST_F(VibratorAgentSeekTest, SeekTimeIsZero, TestSize.Level1)
{
    MISC_HILOGI("SeekTimeIsZero in");
    if (IsSupportVibratorCustom()) {
        int32_t delayTime{ -1 };
        int32_t ret = GetDelayTime(delayTime);
        ASSERT_EQ(ret, 0);
        MISC_HILOGD("delayTime:%{public}d", delayTime);
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop_seek.json");
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
            ASSERT_EQ(package.patternNum, 1);
            ASSERT_EQ(package.patterns[0].eventNum, 11);
            // 打印日志：package内容，开始
            PrintVibratorPackageInfo(package, "SeekTimeIsZero");
            // 打印日志：package内容，结束
        }
        ret = FreeVibratorPackage(package);
        ASSERT_EQ(ret, 0);
    } else {
        ASSERT_EQ(0, 0);
    }
}

HWTEST_F(VibratorAgentSeekTest, RemainingDurationGreaterThanMinValue, TestSize.Level1)
{
    MISC_HILOGI("RemainDurationGreaterThanMinValue in");
    if (IsSupportVibratorCustom()) {
        int32_t delayTime{ -1 };
        int32_t ret = GetDelayTime(delayTime);
        ASSERT_EQ(ret, 0);
        MISC_HILOGD("delayTime:%{public}d", delayTime);
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop_seek.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        VibratorPackage seekPackage;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            ret = PreProcess(vfd, package);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(package.patternNum, 1);
            ASSERT_EQ(package.patterns[0].eventNum, 11);
            int32_t seekTime = 510;
            ret = SeekTimeOnPackage(seekTime, package, seekPackage);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(seekPackage.patternNum, 1);
            ASSERT_EQ(seekPackage.patterns[0].eventNum, 4);
            // 打印日志：package内容，开始
            PrintVibratorPackageInfo(seekPackage, "RemainDurationGreaterThanMinValue");
            // 打印日志：package内容，结束
        }
        ret = FreeVibratorPackage(package);
        ASSERT_EQ(ret, 0);
        ret = FreeVibratorPackage(seekPackage);
        ASSERT_EQ(ret, 0);
    } else {
        ASSERT_EQ(0, 0);
    }
}

HWTEST_F(VibratorAgentSeekTest, RemainingDurationEqualMinValue, TestSize.Level1)
{
    MISC_HILOGI("RemainingDurationEqualMinValue in");
    if (IsSupportVibratorCustom()) {
        int32_t delayTime{ -1 };
        int32_t ret = GetDelayTime(delayTime);
        ASSERT_EQ(ret, 0);
        MISC_HILOGD("delayTime:%{public}d", delayTime);
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop_seek.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        VibratorPackage seekPackage;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            ret = PreProcess(vfd, package);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(package.patternNum, 1);
            ASSERT_EQ(package.patterns[0].eventNum, 11);
            int32_t seekTime = 564;
            ret = SeekTimeOnPackage(seekTime, package, seekPackage);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(seekPackage.patternNum, 1);
            ASSERT_EQ(seekPackage.patterns[0].eventNum, 4);
            // 打印日志：package内容，开始
            PrintVibratorPackageInfo(seekPackage, "RemainingDurationEqualMinValue");
            // 打印日志：package内容，结束
        }
        ret = FreeVibratorPackage(package);
        ASSERT_EQ(ret, 0);
        ret = FreeVibratorPackage(seekPackage);
        ASSERT_EQ(ret, 0);
    } else {
        ASSERT_EQ(0, 0);
    }
}

HWTEST_F(VibratorAgentSeekTest, RemainingDurationLessThanMinValue, TestSize.Level1)
{
    MISC_HILOGI("RemainingDurationLessThanMinValue in");
    if (IsSupportVibratorCustom()) {
        int32_t delayTime{ -1 };
        int32_t ret = GetDelayTime(delayTime);
        ASSERT_EQ(ret, 0);
        MISC_HILOGD("delayTime:%{public}d", delayTime);
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop_seek.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        VibratorPackage seekPackage;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            ret = PreProcess(vfd, package);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(package.patternNum, 1);
            ASSERT_EQ(package.patterns[0].eventNum, 11);
            int32_t seekTime = 565;
            ret = SeekTimeOnPackage(seekTime, package, seekPackage);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(seekPackage.patternNum, 1);
            ASSERT_EQ(seekPackage.patterns[0].eventNum, 3);
            // 打印日志：package内容，开始
            PrintVibratorPackageInfo(seekPackage, "RemainingDurationLessThanMinValue");
            // 打印日志：package内容，结束
        }
        ret = FreeVibratorPackage(package);
        ASSERT_EQ(ret, 0);
        ret = FreeVibratorPackage(seekPackage);
        ASSERT_EQ(ret, 0);
    } else {
        ASSERT_EQ(0, 0);
    }
}

HWTEST_F(VibratorAgentSeekTest, SeekTimePatternIsZero, TestSize.Level1)
{
    MISC_HILOGI("SeekTimePatternIsZero in");
    if (IsSupportVibratorCustom()) {
        int32_t delayTime{ -1 };
        int32_t ret = GetDelayTime(delayTime);
        ASSERT_EQ(ret, 0);
        MISC_HILOGD("delayTime:%{public}d", delayTime);
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop_seek.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        VibratorPackage seekPackage;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            ret = PreProcess(vfd, package);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(package.patternNum, 1);
            ASSERT_EQ(package.patterns[0].eventNum, 11);
            int32_t seekTime = 1001;
            ret = SeekTimeOnPackage(seekTime, package, seekPackage);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(seekPackage.patternNum, 0);
            // 打印日志：package内容，开始
            MISC_HILOGD("SeekTimePatternIsZero data, package patternNum:%{public}d", seekPackage.patternNum);
            // 打印日志：package内容，结束
        }
        ret = FreeVibratorPackage(package);
        ASSERT_EQ(ret, 0);
        ret = FreeVibratorPackage(seekPackage);
        ASSERT_NE(ret, 0);
    } else {
        ASSERT_EQ(0, 0);
    }
}

HWTEST_F(VibratorAgentSeekTest, SeekTimeOnBetweenEvents, TestSize.Level1)
{
    MISC_HILOGI("SeekTimeOnBetweenEvents in");
    if (IsSupportVibratorCustom()) {
        int32_t delayTime{ -1 };
        int32_t ret = GetDelayTime(delayTime);
        ASSERT_EQ(ret, 0);
        MISC_HILOGD("delayTime:%{public}d", delayTime);
        FileDescriptor fileDescriptor("/data/test/vibrator/coin_drop_seek.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        VibratorPackage seekPackage;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            ret = PreProcess(vfd, package);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(package.patternNum, 1);
            ASSERT_EQ(package.patterns[0].eventNum, 11);
            int32_t seekTime = 930;
            ret = SeekTimeOnPackage(seekTime, package, seekPackage);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(seekPackage.patternNum, 1);
            ASSERT_EQ(seekPackage.patterns[0].eventNum, 2);
            ASSERT_EQ(seekPackage.patterns[0].events[0].time, 10);
            // 打印日志：package内容，开始
            PrintVibratorPackageInfo(seekPackage, "SeekTimeOnBetweenEvents");
            // 打印日志：package内容，结束
        }
        ret = FreeVibratorPackage(package);
        ASSERT_EQ(ret, 0);
        ret = FreeVibratorPackage(seekPackage);
        ASSERT_EQ(ret, 0);
    } else {
        ASSERT_EQ(0, 0);
    }
}

HWTEST_F(VibratorAgentSeekTest, SeekTimeOnThirdPattern, TestSize.Level1)
{
    MISC_HILOGI("SeekTimeOnThirdPattern in");
    if (IsSupportVibratorCustom()) {
        int32_t delayTime{ -1 };
        int32_t ret = GetDelayTime(delayTime);
        ASSERT_EQ(ret, 0);
        MISC_HILOGD("delayTime:%{public}d", delayTime);
        FileDescriptor fileDescriptor("/data/test/vibrator/test_128_event.json");
        MISC_HILOGD("fd:%{public}d", fileDescriptor.fd);
        VibratorFileDescription vfd;
        VibratorPackage package;
        VibratorPackage seekPackage;
        struct stat64 statbuf = { 0 };
        if (fstat64(fileDescriptor.fd, &statbuf) == 0) {
            vfd.fd = fileDescriptor.fd;
            vfd.offset = 0;
            vfd.length = statbuf.st_size;
            ret = PreProcess(vfd, package);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(package.patternNum, 8);
            ASSERT_EQ(package.patterns[0].eventNum, 16);
            int32_t seekTime = 1650;
            ret = SeekTimeOnPackage(seekTime, package, seekPackage);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(seekPackage.patternNum, 6);
            ASSERT_EQ(seekPackage.patterns[0].eventNum, 15);
            // 打印日志：package内容，开始
            PrintVibratorPackageInfo(seekPackage, "SeekTimeOnThirdPattern");
            // 打印日志：package内容，结束
        }
        ret = FreeVibratorPackage(package);
        ASSERT_EQ(ret, 0);
        ret = FreeVibratorPackage(seekPackage);
        ASSERT_EQ(ret, 0);
    } else {
        ASSERT_EQ(0, 0);
    }
}
} // namespace Sensors
} // namespace OHOS
