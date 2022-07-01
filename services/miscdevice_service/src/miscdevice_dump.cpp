/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "miscdevice_dump.h"

#include <getopt.h>

#include <cinttypes>
#include <cstring>
#include <ctime>

#include "securec.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "MiscdeviceDump" };
constexpr uint32_t MAX_DUMP_RECORD_SIZE = 30;
constexpr uint32_t BASE_YEAR = 1900;
constexpr uint32_t BASE_MON = 1;
constexpr int32_t INVALID_PID = -1;
constexpr int32_t INVALID_UID = -1;
}  // namespace

void MiscdeviceDump::ParseCommand(int32_t fd, const std::vector<std::string>& args)
{
    int32_t optionIndex = 0;
    struct option dumpOptions[] = {
        {"record", no_argument, 0, 'r'},
        {"help", no_argument, 0, 'h'},
        {NULL, 0, 0, 0}
    };
    char **argv = new char *[args.size()];
    for (size_t i = 0; i < args.size(); ++i) {
        argv[i] = new char[args[i].size() + 1];
        if (strcpy_s(argv[i], args[i].size() + 1, args[i].c_str()) != EOK) {
            MISC_HILOGE("strcpy_s error");
            goto RELEASE_RES;
            return;
        }
    }
    optind = 1;
    int32_t c;
    while ((c = getopt_long(args.size(), argv, "rh", dumpOptions, &optionIndex)) != -1) {
        switch (c) {
            case 'r': {
                DumpMiscdeviceRecord(fd);
                break;
            }
            case 'h': {
                DumpHelp(fd);
                break;
            }
            default: {
                dprintf(fd, "Unrecognized option: %s\nMore info with: \"hidumper -s 3602 -a -h\"\n", argv[optind-1]);
                break;
            }
        }
    }
    RELEASE_RES:
    for (size_t i = 0; i < args.size(); ++i) {
        delete[] argv[i];
    }
    delete[] argv;
}

void MiscdeviceDump::DumpHelp(int32_t fd)
{
    dprintf(fd, "Usage:\n");
    dprintf(fd, "      -h, --help: dump help\n");
    dprintf(fd, "      -r, --record: dump the list of vibrate recorded\n");
}

void MiscdeviceDump::DumpMiscdeviceRecord(int32_t fd)
{
    std::lock_guard<std::mutex> queueLock(recordQueueMutex_);
    if (dumpQueue_.empty()) {
        MISC_HILOGW("dumpQueue_ is empty");
        return;
    }
    size_t length = dumpQueue_.size() > MAX_DUMP_RECORD_SIZE ? MAX_DUMP_RECORD_SIZE : dumpQueue_.size();
    for (size_t i = 0; i < length; ++i) {
        auto record = dumpQueue_.front();
        CHKPV(record);
        dumpQueue_.push(record);
        dumpQueue_.pop();
        if (record->mode == "duration") {
            dprintf(fd, "startTime:%s | uid:%d | pid:%d | mode:%s | duration:%8u | packageName:%s\n",
                record->startTime.c_str(), record->uid, record->pid, record->mode.c_str(), record->duration,
                record->packageName.c_str());
        } else {
            dprintf(fd, "startTime:%s | uid:%d | pid:%d | mode:%s | effect:%s | packageName:%s\n",
                record->startTime.c_str(), record->uid, record->pid, record->mode.c_str(), record->effect.c_str(),
                record->packageName.c_str());
        }
    }
}

void MiscdeviceDump::DumpCurrentTime(std::string &startTime)
{
    timespec curTime;
    clock_gettime(CLOCK_REALTIME, &curTime);
    struct tm *timeinfo = localtime(&(curTime.tv_sec));
    CHKPV(timeinfo);
    startTime.append(std::to_string(timeinfo->tm_year + BASE_YEAR)).append("-")
        .append(std::to_string(timeinfo->tm_mon + BASE_MON)).append("-").append(std::to_string(timeinfo->tm_mday))
        .append(" ").append(std::to_string(timeinfo->tm_hour)).append(":").append(std::to_string(timeinfo->tm_min))
        .append(":").append(std::to_string(timeinfo->tm_sec)).append(".")
        .append(std::to_string(curTime.tv_nsec / (1000 * 1000)));
}

std::string MiscdeviceDump::GetPackageName(AccessTokenID tokenId)
{
    std::string packageName;
    int32_t tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    switch (tokenType) {
        case ATokenTypeEnum::TOKEN_NATIVE: {
            NativeTokenInfo tokenInfo;
            if (AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo) != 0) {
                MISC_HILOGE("get native token info fail");
                return {};
            }
            packageName = tokenInfo.processName;
            break;
        }
        case ATokenTypeEnum::TOKEN_HAP: {
            HapTokenInfo hapInfo;
            if (AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo) != 0) {
                MISC_HILOGE("get hap token info fail");
                return {};
            }
            packageName = hapInfo.bundleName;
            break;
        }
        default: {
            MISC_HILOGW("token type not match");
            break;
        }
    }
    return packageName;
}

void MiscdeviceDump::UpdateRecordQueue(std::shared_ptr<VibrateRecord> record)
{
    std::lock_guard<std::mutex> queueLock(recordQueueMutex_);
    dumpQueue_.push(record);
    if (dumpQueue_.size() > MAX_DUMP_RECORD_SIZE) {
        dumpQueue_.pop();
    }
}

void MiscdeviceDump::SaveVibrator(AccessTokenID callerToken, int32_t uid, int32_t pid, uint32_t timeOut)
{
    if ((uid == INVALID_UID) || (pid <= INVALID_PID)) {
        MISC_HILOGE("uid or pid is invalid");
        return;
    }
    auto record = std::make_shared<VibrateRecord>();
    CHKPV(record);
    record->uid = uid;
    record->pid = pid;
    record->packageName = GetPackageName(callerToken);
    record->duration = timeOut;
    record->mode = "duration";
    DumpCurrentTime(record->startTime);
    UpdateRecordQueue(record);
}

void MiscdeviceDump::SaveVibratorEffect(AccessTokenID callerToken, int32_t uid, int32_t pid,
    const std::string &effect)
{
    if ((uid == INVALID_UID) || (pid <= INVALID_PID)) {
        MISC_HILOGE("uid or pid is invalid");
        return;
    }
    auto record = std::make_shared<VibrateRecord>();
    CHKPV(record);
    record->uid = uid;
    record->pid = pid;
    record->packageName = GetPackageName(callerToken);
    record->effect = effect;
    record->mode = "preset";
    DumpCurrentTime(record->startTime);
    UpdateRecordQueue(record);
}
}  // namespace Sensors
}  // namespace OHOS
