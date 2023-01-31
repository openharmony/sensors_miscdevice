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
#include "file_utils.h"

#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>
#include "securec.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "MiscdeviceFileUtils" };
const std::string CONFIG_DIR = "/vendor/etc/vibrator/";
constexpr int32_t FILE_SIZE_MAX = 0x5000;
constexpr int32_t READ_DATA_BUFF_SIZE = 256;
constexpr int32_t INVALID_FILE_SIZE = -1;
constexpr int32_t FILE_PATH_MAX = 1024;
}  // namespace

std::string ReadJsonFile(const std::string &filePath)
{
    if (filePath.empty()) {
        MISC_HILOGE("Path is empty");
        return {};
    }
    char realPath[PATH_MAX] = {};
    if (realpath(filePath.c_str(), realPath) == nullptr) {
        MISC_HILOGE("Path is error, %{public}d", errno);
        return {};
    }
    if (!CheckFileDir(realPath, CONFIG_DIR)) {
        MISC_HILOGE("File dir is invalid");
        return {};
    }
    if (!CheckFileExtendName(realPath, "json")) {
        MISC_HILOGE("Unable to parse files other than json format");
        return {};
    }
    if (!IsFileExists(filePath)) {
        MISC_HILOGE("File not exist");
        return {};
    }
    if (!CheckFileSize(filePath)) {
        MISC_HILOGE("File size out of read range");
        return {};
    }
    return ReadFile(realPath);
}

int32_t GetFileSize(const std::string& filePath)
{
    struct stat statbuf = {0};
    if (stat(filePath.c_str(), &statbuf) != 0) {
        MISC_HILOGE("Get file size error");
        return INVALID_FILE_SIZE;
    }
    return statbuf.st_size;
}

int32_t GetFileSize(int32_t fd)
{
    if (fd < 0) {
        MISC_HILOGE("fd is invalid, fd:%{public}d", fd);
        return INVALID_FILE_SIZE;
    }
    struct stat statbuf = {0};
    if (fstat(fd, &statbuf) != 0) {
        MISC_HILOGE("fstat error, errno:%{public}d", errno);
        return INVALID_FILE_SIZE;
    }
    return statbuf.st_size;
}

bool CheckFileDir(const std::string& filePath, const std::string& dir)
{
    if (filePath.compare(0, CONFIG_DIR.size(), CONFIG_DIR) != 0) {
        MISC_HILOGE("FilePath dir is invalid");
        return false;
    }
    return true;
}

bool CheckFileSize(const std::string& filePath)
{
    int32_t fileSize = GetFileSize(filePath);
    if ((fileSize <= 0) || (fileSize > FILE_SIZE_MAX)) {
        MISC_HILOGE("File size out of read range");
        return false;
    }
    return true;
}

bool CheckFileExtendName(const std::string& filePath, const std::string& checkExtension)
{
    std::string::size_type pos = filePath.find_last_of('.');
    if (pos == std::string::npos) {
        MISC_HILOGE("File is not find extension");
        return false;
    }
    return (filePath.substr(pos + 1, filePath.npos) == checkExtension);
}

bool IsFileExists(const std::string& fileName)
{
    return (access(fileName.c_str(), F_OK) == 0);
}

std::string ReadFile(const std::string &filePath)
{
    FILE* fp = fopen(filePath.c_str(), "r");
    CHKPS(fp);
    std::string dataStr;
    char buf[READ_DATA_BUFF_SIZE] = { '\0' };
    while (fgets(buf, sizeof(buf), fp) != nullptr) {
        dataStr += buf;
    }
    if (fclose(fp) != 0) {
        MISC_HILOGW("Close file failed");
    }
    return dataStr;
}

std::string ReadFd(int32_t fd)
{
    if (fd < 0) {
        MISC_HILOGE("fd is invalid, fd:%{public}d", fd);
        return "";
    }
    int32_t dupFd = dup(fd);
    FILE* fp = fdopen(dupFd, "r");
    fseek(fp, 0L, SEEK_SET);
    CHKPS(fp);
    std::string dataStr;
    char buf[READ_DATA_BUFF_SIZE] = { '\0' };
    while (fgets(buf, sizeof(buf), fp) != nullptr) {
        dataStr += buf;
    }
    if (fclose(fp) != 0) {
        MISC_HILOGW("Close file failed, errno:%{public}d", errno);
    }
    return dataStr;
}

std::string GetFileSuffix(int32_t fd)
{
    char buf[FILE_PATH_MAX] = { '\0' };
    char filePath[FILE_PATH_MAX] = { '\0' };
    if (snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "/proc/self/fd/%d", fd) < 0) {
        MISC_HILOGE("snprintf_s failed, errno:%{public}d", errno);
        return {};
    }
    MISC_HILOGI("buf : %{public}s", buf);
    if (readlink(buf, filePath, sizeof(filePath) - 1) < 0) {
        MISC_HILOGE("readlink failed");
        return {};
    }
    std::string fileAbsolutePath(filePath);
    MISC_HILOGI("fileAbsolutePath : %{public}s", fileAbsolutePath.c_str());
    return fileAbsolutePath.substr(fileAbsolutePath.find_last_of('.') + 1);
}
}  // namespace Sensors
}  // namespace OHOS
