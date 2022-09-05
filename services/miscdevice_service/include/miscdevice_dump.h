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

#ifndef MISCDEVICE_DUMP_H
#define MISCDEVICE_DUMP_H

#include <queue>

#include "singleton.h"

#include "nocopyable.h"

namespace OHOS {
namespace Sensors {
struct VibrateRecord {
    std::string startTime;
    int32_t duration = 0;
    int32_t uid = 0;
    int32_t pid = 0;
    std::string mode;
    std::string effect;
    std::string packageName;
};
class MiscdeviceDump : public Singleton<MiscdeviceDump> {
public:
    MiscdeviceDump() = default;
    ~MiscdeviceDump() = default;
    void DumpHelp(int32_t fd);
    void DumpMiscdeviceRecord(int32_t fd);
    void ParseCommand(int32_t fd, const std::vector<std::string>& args);
    void SaveVibrator(const std::string &name, int32_t uid, int32_t pid, int32_t timeOut);
    void SaveVibratorEffect(const std::string &name, int32_t uid, int32_t pid, const std::string &effect);

private:
    DISALLOW_COPY_AND_MOVE(MiscdeviceDump);
    std::queue<std::shared_ptr<VibrateRecord>> dumpQueue_;
    std::mutex recordQueueMutex_;
    void DumpCurrentTime(std::string &startTime);
    void UpdateRecordQueue(std::shared_ptr<VibrateRecord> record);
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // MISCDEVICE_DUMP_H
