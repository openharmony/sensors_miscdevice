/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "vibrator_test_common.h"

#include <thread>

#include "atm_tools_param_info.h"
#include "gtest/gtest.h"

#undef LOG_TAG
#define LOG_TAG "VibratorTestCommon"

namespace OHOS {
namespace Sensors {
std::mutex g_lockSetToken;
uint64_t g_shellTokenId = 0;

void VibratorTestCommon::SetTestEvironment(uint64_t shellTokenId)
{
    std::lock_guard<std::mutex> lock(g_lockSetToken);
    g_shellTokenId = shellTokenId;
}

void VibratorTestCommon::ResetTestEvironment()
{
    std::lock_guard<std::mutex> lock(g_lockSetToken);
    g_shellTokenId = 0;
}

uint64_t VibratorTestCommon::GetShellTokenId()
{
    std::lock_guard<std::mutex> lock(g_lockSetToken);
    return g_shellTokenId;
}

int32_t VibratorTestCommon::AllocTestHapToken(
    const HapInfoParams& hapInfo, HapPolicyParams& hapPolicy, AccessTokenIDEx& tokenIdEx)
{
    uint64_t selfTokenId = GetSelfTokenID();
    for (auto& permissionStateFull : hapPolicy.permStateList) {
        PermissionDef permDefResult;
        if (AccessTokenKit::GetDefPermission(permissionStateFull.permissionName, permDefResult) != RET_SUCCESS) {
            continue;
        }
        if (permDefResult.availableLevel > hapPolicy.apl) {
            hapPolicy.aclRequestedList.emplace_back(permissionStateFull.permissionName);
        }
    }
    if (VibratorTestCommon::GetNativeTokenIdFromProcess("foundation") == selfTokenId) {
        return AccessTokenKit::InitHapToken(hapInfo, hapPolicy, tokenIdEx);
    }

    // set sh token for self
    MockNativeToken mock("foundation");
    int32_t ret = AccessTokenKit::InitHapToken(hapInfo, hapPolicy, tokenIdEx);

    // restore
    EXPECT_EQ(0, SetSelfTokenID(selfTokenId));

    return ret;
}

AccessTokenIDEx VibratorTestCommon::AllocAndGrantHapTokenByTest(const HapInfoParams& info, HapPolicyParams& policy)
{
    for (const auto& permissionStateFull : policy.permStateList) {
        PermissionDef permDefResult;
        if (AccessTokenKit::GetDefPermission(permissionStateFull.permissionName, permDefResult) != RET_SUCCESS) {
            continue;
        }
        if (permDefResult.availableLevel > policy.apl) {
            policy.aclRequestedList.emplace_back(permissionStateFull.permissionName);
        }
    }

    AccessTokenIDEx tokenIdEx = {0};
    EXPECT_EQ(RET_SUCCESS, VibratorTestCommon::AllocTestHapToken(info, policy, tokenIdEx));
    AccessTokenID tokenId = tokenIdEx.tokenIdExStruct.tokenID;
    EXPECT_NE(tokenId, INVALID_TOKENID);

    for (const auto& permissionStateFull : policy.permStateList) {
        if (permissionStateFull.grantStatus[0] == PERMISSION_GRANTED) {
            VibratorTestCommon::GrantPermissionByTest(
                tokenId, permissionStateFull.permissionName, permissionStateFull.grantFlags[0]);
        } else {
            VibratorTestCommon::RevokePermissionByTest(
                tokenId, permissionStateFull.permissionName, permissionStateFull.grantFlags[0]);
        }
    }
    return tokenIdEx;
}

int32_t VibratorTestCommon::DeleteTestHapToken(AccessTokenID tokenID)
{
    uint64_t selfTokenId = GetSelfTokenID();
    if (VibratorTestCommon::GetNativeTokenIdFromProcess("foundation") == selfTokenId) {
        return AccessTokenKit::DeleteToken(tokenID);
    }

    // set sh token for self
    MockNativeToken mock("foundation");

    int32_t ret = AccessTokenKit::DeleteToken(tokenID);
    // restore
    EXPECT_EQ(0, SetSelfTokenID(selfTokenId));
    return ret;
}

int32_t VibratorTestCommon::GrantPermissionByTest(AccessTokenID tokenID, const std::string& permission, uint32_t flag)
{
    std::vector<std::string> reqPerm;
    reqPerm.emplace_back("ohos.permission.GRANT_SENSITIVE_PERMISSIONS");
    MockHapToken mock("AccessTokenTestGrant", reqPerm);
    return AccessTokenKit::GrantPermission(tokenID, permission, flag);
}

int32_t VibratorTestCommon::RevokePermissionByTest(AccessTokenID tokenID, const std::string& permission, uint32_t flag)
{
    std::vector<std::string> reqPerm;
    reqPerm.emplace_back("ohos.permission.REVOKE_SENSITIVE_PERMISSIONS");
    MockHapToken mock("AccessTokenTestRevoke", reqPerm);
    return AccessTokenKit::RevokePermission(tokenID, permission, flag);
}

int32_t VibratorTestCommon::ClearUserGrantedPermissionStateByTest(AccessTokenID tokenID)
{
    std::vector<std::string> reqPerm;
    reqPerm.emplace_back("ohos.permission.REVOKE_SENSITIVE_PERMISSIONS");
    MockHapToken mock("AccessTokenTestRevoke", reqPerm);
    return AccessTokenKit::ClearUserGrantedPermissionState(tokenID);
}

AccessTokenID VibratorTestCommon::GetNativeTokenIdFromProcess(const std::string &process)
{
    uint64_t selfTokenId = GetSelfTokenID();
    EXPECT_EQ(0, SetSelfTokenID(VibratorTestCommon::GetShellTokenId())); // set shell token

    std::string dumpInfo;
    AtmToolsParamInfo info;
    info.processName = process;
    AccessTokenKit::DumpTokenInfo(info, dumpInfo);
    size_t pos = dumpInfo.find("\"tokenID\": ");
    if (pos == std::string::npos) {
        return 0;
    }
    pos += std::string("\"tokenID\": ").length();
    std::string numStr;
    while (pos < dumpInfo.length() && std::isdigit(dumpInfo[pos])) {
        numStr += dumpInfo[pos];
        ++pos;
    }
    // restore
    EXPECT_EQ(0, SetSelfTokenID(selfTokenId));

    std::istringstream iss(numStr);
    AccessTokenID tokenID;
    iss >> tokenID;
    return tokenID;
}

// need call by native process
AccessTokenIDEx VibratorTestCommon::GetHapTokenIdFromBundle(
    int32_t userID, const std::string& bundleName, int32_t instIndex)
{
    uint64_t selfTokenId = GetSelfTokenID();
    ATokenTypeEnum type = AccessTokenKit::GetTokenTypeFlag(static_cast<AccessTokenID>(selfTokenId));
    if (type != TOKEN_NATIVE) {
        AccessTokenID tokenId1 = GetNativeTokenIdFromProcess("accesstoken_service");
        EXPECT_EQ(0, SetSelfTokenID(tokenId1));
    }
    AccessTokenIDEx tokenIdEx = AccessTokenKit::GetHapTokenIDEx(userID, bundleName, instIndex);

    EXPECT_EQ(0, SetSelfTokenID(selfTokenId));
    return tokenIdEx;
}


int32_t VibratorTestCommon::GetReqPermissionsByTest(
    AccessTokenID tokenID, std::vector<PermissionStateFull>& permStatList, bool isSystemGrant)
{
    std::vector<std::string> reqPerm;
    reqPerm.emplace_back("ohos.permission.GET_SENSITIVE_PERMISSIONS");
    MockHapToken mock("GetReqPermissionsByTest", reqPerm);
    return AccessTokenKit::GetReqPermissions(tokenID, permStatList, isSystemGrant);
}

int32_t VibratorTestCommon::GetPermissionFlagByTest(AccessTokenID tokenID, const std::string& permission,
    uint32_t& flag)
{
    std::vector<std::string> reqPerm;
    reqPerm.emplace_back("ohos.permission.GET_SENSITIVE_PERMISSIONS");
    MockHapToken mock("GetPermissionFlagByTest", reqPerm, true);
    return AccessTokenKit::GetPermissionFlag(tokenID, permission, flag);
}

MockNativeToken::MockNativeToken(const std::string& process)
{
    selfToken_ = GetSelfTokenID();
    uint32_t tokenId = VibratorTestCommon::GetNativeTokenIdFromProcess(process);
    SetSelfTokenID(tokenId);
}

MockNativeToken::~MockNativeToken()
{
    SetSelfTokenID(selfToken_);
}

MockHapToken::MockHapToken(
    const std::string& bundle, const std::vector<std::string>& reqPerm, bool isSystemApp)
{
    selfToken_ = GetSelfTokenID();
    HapInfoParams infoParams = {
        .userID = 0,
        .bundleName = bundle,
        .instIndex = 0,
        .appIDDesc = "AccessTokenTestAppID",
        .apiVersion = VibratorTestCommon::DEFAULT_API_VERSION,
        .isSystemApp = isSystemApp,
        .appDistributionType = "",
    };

    HapPolicyParams policyParams = {
        .apl = APL_NORMAL,
        .domain = "accesstoken_test_domain",
    };
    for (size_t i = 0; i < reqPerm.size(); ++i) {
        PermissionDef permDefResult;
        if (AccessTokenKit::GetDefPermission(reqPerm[i], permDefResult) != RET_SUCCESS) {
            continue;
        }
        PermissionStateFull permState = {
            .permissionName = reqPerm[i],
            .isGeneral = true,
            .resDeviceID = {"local3"},
            .grantStatus = {PermissionState::PERMISSION_DENIED},
            .grantFlags = {PermissionFlag::PERMISSION_DEFAULT_FLAG}
        };
        policyParams.permStateList.emplace_back(permState);
        if (permDefResult.availableLevel > policyParams.apl) {
            policyParams.aclRequestedList.emplace_back(reqPerm[i]);
        }
    }

    AccessTokenIDEx tokenIdEx = {0};
    EXPECT_EQ(RET_SUCCESS, VibratorTestCommon::AllocTestHapToken(infoParams, policyParams, tokenIdEx));
    mockToken_= tokenIdEx.tokenIdExStruct.tokenID;
    EXPECT_NE(mockToken_, INVALID_TOKENID);
    EXPECT_EQ(0, SetSelfTokenID(tokenIdEx.tokenIDEx));
}

MockHapToken::~MockHapToken()
{
    if (mockToken_ != INVALID_TOKENID) {
        EXPECT_EQ(0, VibratorTestCommon::DeleteTestHapToken(mockToken_));
    }
    EXPECT_EQ(0, SetSelfTokenID(selfToken_));
}
} // namespace Sensors
} // namespace OHOS