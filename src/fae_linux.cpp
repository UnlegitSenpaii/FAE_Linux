#include <iostream>
#include <vector>
#include <unistd.h>
#include <unordered_map>
#include "helpers/logging.hpp"
#include "helpers/filehelper.hpp"
#include "helpers/patching.hpp"


#define RELEASE_VERSION 1

/*
 * note to self: decompiling factorio windows on linux takes ages
 *               it also does on windows :(
 *               haha, not if i close all windows in ghidra
 *
 * Patterns:
 * SteamContext::unlockAchievementsThatAreOnSteamButArentActivatedLocally jz > jnz
 * 74 34 0f 1f 00 48 8b 10 80 7a 3e 00
 * 
 * SteamContext::updateAchievementStatsFromSteam jz > jnz
 * 74 30 0f 1f 80 00 00 00 00 48 8b 10
 *
 * AchievementGui::updateModdedLabel //turn while true to while !true
 * 75 5b 55 45 31 C0 45 31 -- jnz > jz
 *  -> if this doesnt work use: 74 7f 66 0f 1f 44 00 00 48 8b 10 -- jz > jnz
 * 
 * //not needed anymore except if 0x02054feb needs to be changed
 * AchievementGui::AchievementGui jz > jnz
 * 84 8a 03 00 00 0f 1f 44 00
 *
 * ModManager::isVanilla // modifying this directly might break stuff
 * -> this function is used in the linux version in PlayerData to determine if the game is modded or not.
 * In Windows, it's not (or the compiler optimizes it out)
 * Update: this function exits three times now? 
 *  todo: check isVanillaMod & 2x isVanilla
 *
 * note for me: this is the achievements.dat & achievements-modded.dat thingy
 * todo: instead of doing this, just edit achievements-modded.dat to achievements.dat 
 * PlayerData::PlayerData
 * 45 f0 e8 d9 0c           CMOVNZ > CMOVZ
 * 
 * PlayerData::PlayerData 2 changed how it does the achievement check with a null check for *(char *)(global + 0x310)  i think..?  -- prolly not needed
 * 0f 84 35 07 00 00 48 81 c4 e8 26 00 00 -- jz > jnz
 *
 * SteamContext::setStat jz > jmp
 * 74 2d ?? ?? ?? ?? 48 8b 10 80 7a 3e 00 74 17 80 7a 40 00 74 11 80 7a
 *
 * not needed anymore
 * SteamContext::unlockAchievement jz > jmp 
 * 74 2d 0f 1f 40 00 48 8b 10 80 7a 3e 00
 * 
 *
 * AchievementGui::allowed (map) jz > jmp
 * 74 17 48 83 78 20 00
 * 74 10 31 c0 5b 41 5c 41 5d 41 5e
 */

// TODO: Implement placeholders for the patterns
std::unordered_map<std::string, std::vector<uint8_t>> patternsJZToJNZ {
    {"SteamContext::unlockAchievementsThatAreOnSteamButArentActivatedLocally", { 0x74, 0x34, 0x0f, 0x1f, 0x00, 0x48, 0x8b, 0x10, 0x80, 0x7a, 0x3e, 0x00 }},
    {"SteamContext::updateAchievementStatsFromSteam", {0x74, 0x30, 0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8b, 0x10}}, 
};

std::unordered_map<std::string, std::vector<uint8_t>> patternsToJMPFromJNZ {
    {"AchievementGui::updateModdedLabel", {0x75, 0x5b, 0x55, 0x45, 0x31, 0xC0, 0x45, 0x31}},
};

std::unordered_map<std::string, std::vector<uint8_t>> patternsJZToJMP {
    {"SteamContext::setStat & SteamContext::unlockAchievement", {0x74, 0x2d, 0x0f, 0x1f, 0x40, 0x00, 0x48, 0x8b, 0x10, 0x80, 0x7a, 0x3e, 0x00, 0x74, 0x17, 0x80, 0x7a, 0x40, 0x00, 0x74, 0x11, 0x80, 0x7a}},
    {"AchievementGui::allowed", {0x74, 0x17, 0x48, 0x83, 0x78, 0x20, 0x00}},
    {"AchievementGui::allowed2", {0x74, 0x10, 0x31, 0xc0, 0x5b, 0x41, 0x5c, 0x41, 0x5d, 0x41, 0x5e}},
};

std::unordered_map<std::string, std::vector<uint8_t>> patternsToCMOVZ {
    {"PlayerData::PlayerData", {0x45, 0xF0, 0xE8, 0xD9, 0x0C}}
};

//Usage Example: ./FAE_Linux ./factorio

void doPatching(const std::string &factorioFilePath, const std::pair<std::string, std::vector<uint8_t>> &patternPair, int replaceInstruction) {
    const std::string patternName = patternPair.first;
    const std::vector<uint8_t> pattern = patternPair.second;
    const std::vector<uint8_t> replacementPattern = Patcher::GenerateReplacePattern(pattern, replaceInstruction);
    Log::LogF("Patching %s with following data:\n pattern:\t0x%x\n replacement pattern:\t0x%x\n", patternName.c_str(), pattern.data(), replacementPattern.data());
    if (!Patcher::ReplaceHexPattern(factorioFilePath, pattern, replacementPattern)) {
        Log::LogF(" -> FAILED!\n");
        return;
    }
    Log::LogF(" -> SUCCESS!\n");
}

int main(int argc, char *argv[]) {
    Log::PrintAsciiArtWelcome();
    Log::Initialize("./FAE_Debug.log", false);
    Log::LogF("Initialized Logging.\n");

#ifdef RELEASE_VERSION
    if (argc < 2) {
        Log::LogF("Incorrect Usage!\nUsage: %s [Factorio File Path]", argv[0]);
        return 1;
    }
    std::string factorioFilePath = argv[1];
#else
    std::string factorioFilePath = "/home/senpaii/steamdrives/nvme1/SteamLibrary/steamapps/common/Factorio/bin/x64/factorio";
#endif
    if (!FileHelper::DoesFileExist(factorioFilePath)) {
        Log::LogF("The provided filepath is incorrect.\n");
        return 1;
    }
    Log::LogF("Provided path exits.\n");

    // I have a slight suspicion that this can be moved into a loop

    Log::LogF("Patching instructions to JNZ..\n");
    for (auto &patternPair: patternsJZToJNZ) {
        doPatching(factorioFilePath, patternPair, 0);
    }

    Log::LogF("Patching instructions to JMP from JZ..\n");
    for (auto &patternPair: patternsJZToJMP) {
        doPatching(factorioFilePath, patternPair, 1);
    }

    Log::LogF("Patching instructions to JMP from JNZ..\n");
    for (auto &patternPair: patternsToJMPFromJNZ) {
        doPatching(factorioFilePath, patternPair, 2);
    }

    Log::LogF("Patching instructions to CMOVZ from CMOVNZ..\n");
 
    std::string userInput = "";
    Log::LogF("Do you want to use the modded achievement save? (y/N)\n");
    std::getline(std::cin, userInput);

    if (userInput.empty() || std::tolower(userInput[0]) == 'n') {
        Log::LogF("Using vanilla achievements.dat\n");
        for (auto &patternPair : patternsToCMOVZ) {
            doPatching(factorioFilePath, patternPair, 3);
        }
    } else {
        Log::LogF("Using modded achievements.dat\n");
    }

    Log::LogF("Marking Factorio as an executable..\n");
    if (!FileHelper::MarkFileExecutable(factorioFilePath)) {
        Log::LogF("Failed to mark factorio as an executable!\nYou can do this yourself too, with 'chmod +x ./factorio'\n");
        return 1;
    }
#ifdef RELEASE_VERSION
    usleep(2800 * 1000);
    Log::PrintSmugAstolfo();
#endif
    return 0;
}
