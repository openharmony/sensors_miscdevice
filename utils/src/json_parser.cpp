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
#include "json_parser.h"

#include <sys/stat.h>
#include <unistd.h>

#include "file_utils.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MISC_LOG_DOMAIN, "JsonParser" };
}  // namespace

JsonParser::JsonParser(const std::string &filePath)
{
    std::string jsonStr = ReadJsonFile(filePath);
    if (jsonStr.empty()) {
        MISC_HILOGE("Read json file fail");
        return;
    }
    cJson_ = cJSON_Parse(jsonStr.c_str());
}

JsonParser::~JsonParser()
{
    if (cJson_ != nullptr) {
        cJSON_Delete(cJson_);
    }
}

cJSON* JsonParser::GetObjectItem(cJSON *json, const std::string& key) const
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

cJSON* JsonParser::GetObjectItem(const std::string& key) const
{
    return GetObjectItem(cJson_, key);
}

int32_t JsonParser::ParseJsonArray(cJSON *json, const std::string& key,
    std::vector<std::string>& vals) const
{
    cJSON* jsonArray = GetObjectItem(json, key);
    if (!cJSON_IsArray(jsonArray)) {
        MISC_HILOGE("The value of %{public}s is not array", key.c_str());
        return ERROR;
    }
    int32_t size = cJSON_GetArraySize(jsonArray);
    for (int32_t i = 0; i < size; ++i) {
        cJSON* val = cJSON_GetArrayItem(jsonArray, i);
        if (!cJSON_IsString(val)) {
            MISC_HILOGE("The value of index %{public}d is not string", i);
            return ERROR;
        }
        vals.push_back(val->valuestring);
    }
    return SUCCESS;
}

int32_t JsonParser::ParseJsonArray(const std::string& key, std::vector<std::string>& vals) const
{
    return ParseJsonArray(cJson_, key, vals);
}
}  // namespace Sensors
}  // namespace OHOS