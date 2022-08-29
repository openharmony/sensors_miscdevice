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

#ifndef MISCDEVICE_JSON_PARSER_H
#define MISCDEVICE_JSON_PARSER_H
#include <string>
#include <stdint.h>

#include "cJSON.h"

namespace OHOS {
namespace Sensors {

class MiscdeviceJsonParser {
public:
    explicit MiscdeviceJsonParser(const std::string &filePath);
    ~MiscdeviceJsonParser();
    int32_t ParseJsonArray(cJSON *json, const std::string& key, std::vector<std::string>& vals) const;
    cJSON* GetJsonData() const;
    cJSON* GetObjectItem(cJSON *json, const std::string& key) const;

private:
    cJSON* cJson_ = nullptr;
    std::string filePath_;
    std::string ReadJsonFile();
    bool IsValidPath(const std::string &path);
    bool CheckFileExtendName(const std::string& filePath, const std::string& checkExtension);
    bool IsFileExists(const std::string& fileName);
    std::string ReadFile(const std::string &filePath);
    int32_t GetFileSize(const std::string& filePath);
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // MISCDEVICE_JSON_PARSER_H