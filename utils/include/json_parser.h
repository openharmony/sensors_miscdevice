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

#include <cstdint>
#include <string>
#include <vector>

#include "cJSON.h"

namespace OHOS {
namespace Sensors {
class JsonParser {
public:
    explicit JsonParser(const std::string &filePath);
    ~JsonParser();
    int32_t ParseJsonArray(cJSON *json, const std::string& key, std::vector<std::string>& vals) const;
    int32_t ParseJsonArray(const std::string& key, std::vector<std::string>& vals) const;
    cJSON* GetObjectItem(cJSON *json, const std::string& key) const;
    cJSON* GetObjectItem(const std::string& key) const;

private:
    cJSON* cJson_ = nullptr;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // MISCDEVICE_JSON_PARSER_H