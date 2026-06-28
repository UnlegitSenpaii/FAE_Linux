#include "helpers/filehelper.hpp"
#include "helpers/logging.hpp"
#include "helpers/patching.hpp"
#include <iostream>
#include <unistd.h>
#include <unordered_map>
#include <vector>

/*
 * Want to update the patterns yourself?
 * check out the wiki: https://github.com/UnlegitSenpaii/FAE_Linux/wiki/Finding-the-currently-used-patterns-in-ghidra
 *
 * why is ghidra so slow on linux? zZzZ
 *
 * Notes about pattern:
 *
 * SteamContext::unlockAchievementsThatAreOnSteamButArentActivatedLocally jz > jnz
 * Possibly inlined in OnUserStatsReceived -- Currently: yes
 *
 * SteamContext::updateAchievementStatsFromSteam jz > jnz
 * Possibly inlined in OnUserStatsReceived -- Currently: yes
 *
 * AchievementGui::refresh ---- XREF gui-achievements.modded-game  -- JNZ > JZ
 *
 * PlayerData::PlayerData JZ > jnz
 * this is the achievements.dat & achievements-modded.dat thingy
 * todo: instead of doing this, just edit achievements-modded.dat to achievements.dat
 *
 * SteamContext::setStat jz > jmp
 *
 * SteamContext::unlockAchievement jz > jmp
 *
 * AchievementGui::allowed (map) jz > jmp
 * todo: replace top of function with return true: B8 returnval 00 00 00 C3
 */

std::vector<patternData_t> patternList = {
    /*
        JZ -> JMP Patches
    */
    { PATCH_TYPE_JZJMP, "PlayerData::PlayerData",
        "74 e2 48 89 d3 48 b8 ff ff ff ff ff ff ff 7f", 1, true },

    { PATCH_TYPE_JZJMP, "SteamContext::setStat",
        "74 1a 4c 8b 00 41 80 78 3e 01 75 ed 41 80 78 40 01 75 e6 41 80 78 41" },

    { PATCH_TYPE_JZJMP, "SteamContext::unlockAchievement",
        "74 17 48 8b 10 80 7a 3e 01 75 ee 80 7a 40 01 75 e8 80 7a 41 01 74 e2 eb 3c" },

    { PATCH_TYPE_JZJMP, "AchievementGui::allowed",
        "74 07 48 83 78 20 00 75 cc" },

    /*
        JZ -> JNZ Patches
    */
    { PATCH_TYPE_JZJNZ, "SteamContext::updateAchievementStatsFromSteam",
        "74 ? 48 8B 31 80 7E ? 01 ? ? 80 7E ? 01" },

    { PATCH_TYPE_JZJNZ, "SteamContext::unlockAchievementsThatAreOnSteamButArentActivatedLocally",
        "74 ? 48 ba 74 65 73 74 5f 6d 6f 64 eb ? * 48 83 c0 08" },
        
    { PATCH_TYPE_JZJNZ, "AchievementGui::refresh",
        "84 ? 00 00 00 48 8b 10 80 7a 3e 01 75 ea" },

    /*
        JNZ -> JMP Patches
    */
    { PATCH_TYPE_JNZJMP, "AchievementGui::allowed2",
        "75 cc 49 8b 80 ? 01" },

};

void doPatching(std::vector<std::uint8_t>& buffer, const patternData_t& patternData)
{
    Log::LogF("Patching %s:\n", patternData.patternName.c_str());

    std::vector<std::uint8_t> completeSearchPattern;
    if (!Patcher::GenerateSearchPattern(buffer, patternData.pattern, completeSearchPattern)) {
        Log::LogF(" -> FAILED!\n");
        return;
    }

    const std::vector<uint8_t> replacementPattern = Patcher::GenerateReplacePattern(completeSearchPattern, patternData.patchType);

    std::string completePatternAsString;
    for (const auto& byte : completeSearchPattern) {
        char buffer[4]; // just dont overflow, please
        snprintf(buffer, sizeof(buffer), "%02X ", byte);
        completePatternAsString += buffer;
    }
    Log::LogF("Looking for memory pattern: %s\n", completePatternAsString.c_str());

    if (!Patcher::ReplaceHexPattern(buffer, completeSearchPattern, replacementPattern, patternData.expectedPatchCount)) {
        Log::LogF(" -> FAILED!\n");
        return;
    }
    Log::LogF(" -> SUCCESS!\n");
}

int main(int argc, char* argv[])
{
    Log::PrintAsciiArtWelcome();
    Log::Initialize("./FAE_Debug.log", false);
    Log::LogF("Initialized Logging.\n");

    if (argc < 2) {
        Log::LogF(
            "Incorrect Usage!\n"
            "Usage: %s [Factorio File Path] [--no-prompt]\n"
            "Flags:\n"
            "  --no-prompt    Do not wait for user input before exiting.\n",
            argv[0]);
        return 1;
    }

    bool noPrompt = false;
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--no-prompt") {
            noPrompt = true;
        }
    }

    std::string factorioFilePath = argv[1];

    if (!FileHelper::DoesFileExist(factorioFilePath)) {
        Log::LogF("The provided filepath is incorrect.\n");
        return 1;
    }

    Log::LogF("Reading factorio binary..\n");
    std::vector<std::uint8_t> buffer;
    if (!FileHelper::ReadFileToBuffer(factorioFilePath, buffer)) {
        Log::LogF("Failed to read factorio to buffer.\n");
        return 1;
    }

    for (const auto& patternEntry : patternList) {
        if (!patternEntry.optional) {
            doPatching(buffer, patternEntry);
            continue;
        }

        std::string userInput = "";
        if (noPrompt) {
            Log::LogF("--no-prompt set: using default (vanilla achievements.dat)\n");
        } else {
            Log::LogF("\033[1mDo you want to use the modded achievement save? (y/N)\033[0m\n");
            std::getline(std::cin, userInput);
        }

        if (noPrompt || userInput.empty() || std::tolower(userInput[0]) == 'n') {
            Log::LogF("Using vanilla achievements.dat\n");
            doPatching(buffer, patternEntry);

        } else {
            Log::LogF("Using modded achievements.dat\n");
        }
    }

    Log::LogF("Writing patched factorio binary..\n");
    if (!FileHelper::WriteBufferToFile(factorioFilePath, buffer)) {
        Log::LogF("Failed to write patched data to factorio.\n");
        return 1;
    }

    Log::LogF("Marking Factorio as an executable..\n");
    if (!FileHelper::MarkFileExecutable(factorioFilePath)) {
        Log::LogF("Failed to mark factorio as an executable!\nYou can do this yourself too, with 'chmod +x ./factorio'\n");
        return 1;
    }

    usleep(2800 * 1000);
    Log::PrintSmugAstolfo();
    return 0;
}
