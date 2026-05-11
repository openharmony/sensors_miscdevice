/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>

#include "cJSON.h"
#include "vibrator_agent.h"
#include "vibrator_agent_type.h"

inline void CliLog(char const* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int32_t ret = vfprintf(stdout, fmt, args);
    if (ret >= 0) {
        ret = fprintf(stdout, "\n");
    }
    (void)ret;
    va_end(args);
}

void CliError(char const* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int32_t ret = fprintf(stdout, "[ERROR] ");
    if (ret >= 0) {
        ret = vfprintf(stdout, fmt, args);
    }
    if (ret >= 0) {
        ret = fprintf(stdout, "\n");
    }
    (void)ret;
    va_end(args);
}

constexpr int32_t VIBRATOR_SUCCESS = 0;
constexpr size_t EFFECT_ID_ARG_LEN = 11;
constexpr size_t EQUAL_SIGN_OFFSET = 12;
constexpr size_t MAX_EFFECT_ID_LENGTH = 256;
constexpr int32_t MIN_ARGC_WITH_COMMAND = 2;
constexpr int32_t ARGV_CMD_PARAM_START_INDEX = 2;
constexpr size_t HELP_ARGV_SIZE = 2;

using CommandHandler = std::function<int(int, char**)>;

struct Command {
    const char* name;
    const char* description;
    const char* usage;
    const char* parameters;
    const char* examples;
    CommandHandler handler;
};

static std::unordered_map<std::string, Command> gCommands;
static const char* G_PROGRAM_NAME = "ohos-vibratorControl";
static bool g_hasSubcommand = true;
static const char* G_TOOL_DESCRIPTION = "Vibrator control utility for starting preset vibrations and "
    "checking effect support";

inline void RegisterCommand(Command const& cmd)
{
    gCommands[cmd.name] = cmd;
}

int OutputSuccess(cJSON* data)
{
    cJSON* response = cJSON_CreateObject();
    if (response == nullptr) {
        return 1;
    }

    cJSON_AddItemToObject(response, "type", cJSON_CreateString("result"));
    cJSON_AddItemToObject(response, "status", cJSON_CreateString("success"));
    cJSON_AddItemToObject(response, "data", data);

    char* jsonStr = cJSON_PrintUnformatted(response);
    if (jsonStr != nullptr) {
        std::cout << jsonStr << std::endl;
        free(jsonStr);
    }
    cJSON_Delete(response);
    return std::cout ? 0 : 1;
}

int OutputError(std::string const& code, std::string const& message, std::string const& suggestion = "")
{
    cJSON* response = cJSON_CreateObject();
    if (response == nullptr) {
        return 1;
    }

    cJSON_AddItemToObject(response, "type", cJSON_CreateString("result"));
    cJSON_AddItemToObject(response, "status", cJSON_CreateString("failed"));
    cJSON_AddItemToObject(response, "errCode", cJSON_CreateString(code.c_str()));
    cJSON_AddItemToObject(response, "errMsg", cJSON_CreateString(message.c_str()));
    cJSON_AddItemToObject(response, "suggestion", cJSON_CreateString(suggestion.c_str()));

    char* jsonStr = cJSON_PrintUnformatted(response);
    if (jsonStr != nullptr) {
        std::cout << jsonStr << std::endl;
        free(jsonStr);
    }
    cJSON_Delete(response);
    return 1;
}

std::string ParseEffectId(int32_t argc, char** argv)
{
    std::string effectId;
    for (int32_t i = 0; i < argc; i++) {
        if (strncmp(argv[i], "--effectId", EFFECT_ID_ARG_LEN) == 0) {
            if (argv[i][EFFECT_ID_ARG_LEN] == '=') {
                effectId = argv[i] + EQUAL_SIGN_OFFSET;
            } else if (i + 1 < argc && argv[i + 1][0] != '-') {
                effectId = argv[i + 1];
                i++;
            }
        }
    }
    return effectId;
}

void ShowGeneralHelp()
{
    CliLog("%s - %s", G_PROGRAM_NAME, G_TOOL_DESCRIPTION);
    CliLog("");
    CliLog("Usage:");
    CliLog("  %s [options]", G_PROGRAM_NAME);
    CliLog("  %s <command> [options]", G_PROGRAM_NAME);
    CliLog("");
    CliLog("Parameters:");
    CliLog("  --help             Display this help message");
    CliLog("");
    CliLog("SubCommands:");
    for (const auto& pair : gCommands) {
        CliLog("  %-18s %s", pair.first.c_str(), pair.second.description ? pair.second.description : "");
    }
    CliLog("");
    CliLog("Examples:");
    CliLog("  %s --help", G_PROGRAM_NAME);
    for (const auto& pair : gCommands) {
        if (strcmp(pair.first.c_str(), "help") != 0) {
            CliLog("  %s %s --help", G_PROGRAM_NAME, pair.first.c_str());
            break;
        }
    }
}

int OutputInvalidCommandError(std::string const& targetCmd)
{
    cJSON* response = cJSON_CreateObject();
    if (response == nullptr) {
        return 1;
    }

    cJSON_AddItemToObject(response, "status", cJSON_CreateString("failed"));
    cJSON_AddItemToObject(response, "errCode", cJSON_CreateString("ERR_VIB_INVALID_COMMAND"));
    cJSON_AddItemToObject(response, "errMsg", cJSON_CreateString(("Unknown command: " + targetCmd).c_str()));
    cJSON_AddItemToObject(response, "suggestion",
        cJSON_CreateString("Run 'ohos-vibratorControl --help' to see available commands"));

    char* jsonStr = cJSON_PrintUnformatted(response);
    if (jsonStr != nullptr) {
        std::cout << jsonStr << std::endl;
        free(jsonStr);
    }
    cJSON_Delete(response);
    return 1;
}

void ShowCommandHelp(Command const& cmd)
{
    CliLog("%s %s - %s", G_PROGRAM_NAME, cmd.name, cmd.description ? cmd.description : "N/A");

    if (cmd.usage) {
        CliLog("");
        CliLog("Usage:");
        CliLog("  %s", cmd.usage);
    }

    if (cmd.parameters) {
        CliLog("");
        CliLog("Parameters:");
        CliLog("%s", cmd.parameters);
    }
    CliLog("    %-18s %s", "--help", "Display this help message");

    if (cmd.examples) {
        CliLog("");
        CliLog("Examples:");
        CliLog("%s", cmd.examples);
    }
}

int CmdHelp(int32_t argc, char** argv)
{
    std::string targetCmd;
    for (int32_t i = 1; i < argc; i++) {
        if (argv[i] != nullptr && argv[i][0] != '-') {
            targetCmd = argv[i];
            break;
        }
    }

    if (targetCmd.empty()) {
        ShowGeneralHelp();
        return 0;
    }

    auto it = gCommands.find(targetCmd);
    if (it == gCommands.end()) {
        return OutputInvalidCommandError(targetCmd);
    }

    ShowCommandHelp(it->second);
    return 0;
}

int CmdStartVibrator(int32_t argc, char** argv)
{
    std::string effectId = ParseEffectId(argc, argv);
    if (effectId.empty()) {
        return OutputError("ERR_VIB_ARG_MISSING",
            "Missing required parameter: --effectId",
            "Run 'ohos-vibratorControl startVibrator --help' for usage details");
    }

    if (effectId.length() > MAX_EFFECT_ID_LENGTH) {
        return OutputError("ERR_VIB_ARG_TOO_LONG",
            "EffectId length exceeds maximum limit of 256 characters",
            "Please provide a shorter effect ID");
    }

    int32_t ret = OHOS::Sensors::StartVibrator(effectId.c_str());
    if (ret != VIBRATOR_SUCCESS) {
        return OutputError("ERR_VIB_START_FAILED",
            "Failed to start vibrator with effect: " + effectId,
            "Please check if the effect is supported using "
            "'ohos-vibratorControl isSupportEffect --effectId " + effectId + "'");
    }

    cJSON* result = cJSON_CreateObject();
    if (result == nullptr) {
        return 1;
    }
    cJSON_AddItemToObject(result, "effectId", cJSON_CreateString(effectId.c_str()));
    cJSON_AddItemToObject(result, "status", cJSON_CreateString("vibrating"));
    return OutputSuccess(result);
}

int CmdIsSupportEffect(int32_t argc, char** argv)
{
    std::string effectId = ParseEffectId(argc, argv);
    if (effectId.empty()) {
        return OutputError("ERR_VIB_ARG_MISSING",
            "Missing required parameter: --effectId",
            "Run 'ohos-vibratorControl isSupportEffect --help' for usage details");
    }

    if (effectId.length() > MAX_EFFECT_ID_LENGTH) {
        return OutputError("ERR_VIB_ARG_TOO_LONG",
            "EffectId length exceeds maximum limit of 256 characters",
            "Please provide a shorter effect ID");
    }

    bool state = false;
    int32_t ret = OHOS::Sensors::IsSupportEffect(effectId.c_str(), &state);
    if (ret != VIBRATOR_SUCCESS) {
        return OutputError("ERR_VIB_QUERY_FAILED",
            "Failed to query effect support for: " + effectId,
            "Please check the effect ID format and try again");
    }

    cJSON* result = cJSON_CreateObject();
    if (result == nullptr) {
        return 1;
    }
    cJSON_AddItemToObject(result, "effectId", cJSON_CreateString(effectId.c_str()));
    cJSON_AddItemToObject(result, "isSupported", cJSON_CreateBool(state ? 1 : 0));
    return OutputSuccess(result);
}

void InitCommands()
{
    Command helpCmd = {"help", "Show this help message",
        "ohos-vibratorControl help [command]",
        " command           Optional. Show detailed help for a specific command.",
        " ohos-vibratorControl help\n"
        " ohos-vibratorControl help startVibrator",
        CmdHelp};
    RegisterCommand(helpCmd);

    Command startVibratorCmd = {"startVibrator", "Start preset vibration",
        "ohos-vibratorControl startVibrator --effectId <string>",
        "    --effectId       Required string. The preset vibration effect ID "
        "(e.g., haptic.clock.timer, haptic.long.press)",
        "    ohos-vibratorControl startVibrator --effectId haptic.clock.timer\n"
        "    ohos-vibratorControl startVibrator --effectId haptic.long.press.heavy",
        CmdStartVibrator};
    RegisterCommand(startVibratorCmd);

    Command isSupportEffectCmd = {"isSupportEffect", "Query if a preset effect is supported",
        "ohos-vibratorControl isSupportEffect --effectId <string>",
        "    --effectId       Required string. The preset vibration effect ID to query (e.g., haptic.clock.timer)",
        "    ohos-vibratorControl isSupportEffect --effectId haptic.clock.timer\n"
        "    ohos-vibratorControl isSupportEffect --effectId haptic.long.press.heavy",
        CmdIsSupportEffect};
    RegisterCommand(isSupportEffectCmd);

    g_hasSubcommand = (gCommands.size() > 1);
}

void PrintUsage(const char* prog)
{
    CliError("Usage: %s [options...]", prog);
    CliError("Run '%s help' for more information", prog);
}

int main(int argc, char** argv)
{
    G_PROGRAM_NAME = argv[0];
    if (argc >= MIN_ARGC_WITH_COMMAND && strcmp(argv[1], "--help") == 0) {
        InitCommands();
        char* helpArgv[HELP_ARGV_SIZE] = {argv[0], nullptr};
        CmdHelp(1, helpArgv);
        return 0;
    }

    if (argc < MIN_ARGC_WITH_COMMAND) {
        PrintUsage(argv[0]);
        return 1;
    }

    InitCommands();

    std::string cmdName = argv[1];
    for (int32_t i = ARGV_CMD_PARAM_START_INDEX; i < argc; i++) {
        if (argv[i] != nullptr && strcmp(argv[i], "--help") == 0) {
            char* helpArgv[HELP_ARGV_SIZE] = {argv[0], const_cast<char*>(cmdName.c_str())};
            CmdHelp(MIN_ARGC_WITH_COMMAND, helpArgv);
            return 0;
        }
    }

    auto it = gCommands.find(cmdName);
    if (it == gCommands.end()) {
        CliError("Unknown command: %s", cmdName.c_str());
        PrintUsage(argv[0]);
        return 1;
    }

    return it->second.handler(static_cast<int32_t>(argc - MIN_ARGC_WITH_COMMAND), argv + ARGV_CMD_PARAM_START_INDEX);
}
