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
#include "miscdevice_json_parser.h"

#include <sys/stat.h>
#include <unistd.h>

#include "permission_util.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "MiscdeviceJsonParser" };
const std::string CONFIG_DIR = "/vendor/etc/sensors/";
constexpr int32_t FILE_SIZE_MAX = 0x5000;
constexpr int32_t READ_DATA_BUFF_SIZE = 256;
constexpr int32_t INVALID_FILE_SIZE = -1;
}  // namespace

MiscdeviceJsonParser::MiscdeviceJsonParser(const std::string &filePath)
{
    filePath_ = filePath;
    std::string jsonStr = ReadJsonFile();
    if (jsonStr == "") {
        MISC_HILOGE("read json file fail");
        return;
    }
    cJson_ = cJSON_Parse(jsonStr.c_str());
}

MiscdeviceJsonParser::~MiscdeviceJsonParser() {
    CHKPV(cJson_);
    delete cJson_;
    cJson_ = nullptr;
};

cJSON* MiscdeviceJsonParser::GetJsonData() const
{
    return cJson_;
}

std::string MiscdeviceJsonParser::ReadJsonFile()
{
    if (filePath_.empty()) {
       MISC_HILOGE("path is empty");
        return "";
    }
    char realPath[PATH_MAX] = {};
    if (realpath(filePath_.c_str(), realPath) == nullptr) {
       MISC_HILOGE("path is error");
        return "";
    }
    if (!IsValidPath(realPath)) {
        MISC_HILOGE("path is invalid");
        return "";
    }
    return ReadFile(filePath_);
}

cJSON* MiscdeviceJsonParser::GetObjectItem(cJSON *json, const std::string& key) const
{
    if (!cJSON_IsObject(json)) {
        MISC_HILOGE("The json is not object");
        return nullptr;
    }
    if (!cJSON_HasObjectItem(json, key.c_str())) {
        MISC_HILOGE("The json is not data:%{public}s", key.c_str());
        return nullptr;
    }
    return cJSON_GetObjectItem(json, key.c_str());
}

int32_t MiscdeviceJsonParser::ParseJsonArray(cJSON *json, const std::string& key,
    std::vector<std::string>& vals) const
{
    cJSON* jsonArray = GetObjectItem(json, key);
    if (!cJSON_IsArray(jsonArray)) {
        MISC_HILOGE("The value of %{public}s is not array", key.c_str());
        return ERROR;
    }
    int32_t jsonArraySize = cJSON_GetArraySize(jsonArray);
    for (int32_t i = 0; i < jsonArraySize; ++i) {
        cJSON* val = cJSON_GetArrayItem(jsonArray, i);
        if (cJSON_IsString(val)) {
            vals.push_back(val->valuestring);
        }
    }
    return SUCCESS;
}

int32_t MiscdeviceJsonParser::GetFileSize(const std::string& filePath)
{
    struct stat statbuf = {0};
    if (stat(filePath.c_str(), &statbuf) != 0) {
       MISC_HILOGE("get file size error");
        return INVALID_FILE_SIZE;
    }
    return statbuf.st_size;
}

bool MiscdeviceJsonParser::IsValidPath(const std::string &filePath)
{
    if (filePath.compare(0, CONFIG_DIR.size(), CONFIG_DIR) != 0) {
        MISC_HILOGE("filePath dir is invalid");
        return false;
    }
    if (!CheckFileExtendName(filePath, ".json")) {
        MISC_HILOGE("Unable to parse files other than json format");
        return false;
    }
    if (!IsFileExists(filePath)) {
        MISC_HILOGE("file not exist");
        return false;
    }
    int32_t fileSize = GetFileSize(filePath);
    if ((fileSize <= 0) || (fileSize > FILE_SIZE_MAX)) {
       MISC_HILOGE("file size out of read range");
        return false;
    }
    return true;
}

bool MiscdeviceJsonParser::CheckFileExtendName(const std::string& filePath, const std::string& checkExtension)
{
    std::string::size_type pos = filePath.find_last_of('.');
    if (pos == std::string::npos) {
       MISC_HILOGE("file is not find extension");
        return false;
    }
    return (filePath.substr(pos + 1, filePath.npos) == checkExtension);
}

bool MiscdeviceJsonParser::IsFileExists(const std::string& fileName)
{
    return (access(fileName.c_str(), F_OK) == 0);
}

std::string MiscdeviceJsonParser::ReadFile(const std::string &filePath)
{
    FILE* fp = fopen(filePath.c_str(), "r");
    CHKPS(fp);
    std::string dataStr;
    char buf[READ_DATA_BUFF_SIZE] = {};
    while (fgets(buf, sizeof(buf), fp) != nullptr) {
        dataStr += buf;
    }
    if (fclose(fp) != 0) {
       MISC_HILOGW("close file failed");
    }
    return dataStr;
}
}  // namespace Sensors
}  // namespace OHOS