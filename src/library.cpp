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
 *
 * Patterns:
 * SteamContext::onUserStatsReceived jz > jnz
 * 74 73 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 48 8b 10
 *
 * AchievementGui::AchievementGui jz > jnz
 * 84 8a 03 00 00 0f 1f 44 00
 *
 * ModManager::isVanilla // modifying this directly might break stuff
 * -> this function is used in the linux version in PlayerData to determine if the game is modded or not.
 * In Windows, it's not (or the compiler optimizes it out)
 *
 * PlayerData::PlayerData CMOVNZ > CMOVZ -- Prefix (0f) discarded!!
 * 45 f0 e8 12 e7 2f 01
 *
 * SteamContext::setStat jz > jmp
 * 74 2d ?? ?? ?? ?? 48 8b 10 80 7a 3e 00 74 17 80 7a 40 00 74 11 80 7a
 *
 * SteamContext::unlockAchievement jz > jmp
 * 74 29 48 8b 10 80 7a 3e 00 74 17
 *
 * AchievementGui::allowed (map) jnz > jmp
 * 75 86 80 bb ef 01 00 00 00 0f 85 79 ff ff ff
 */
// TODO: Implement placeholders for the patterns
std::unordered_map<std::string, std::vector<uint8_t>> patternsToJNZ {
    {"SteamContext::onUserStatsReceived", {0x74, 0x73, 0x66, 0x2e, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8b, 0x10}},
    {"AchievementGui::AchievementGui", {0x84, 0x8a, 0x03, 0x00, 0x00, 0x0f, 0x1f, 0x44, 0x00}}
};

std::unordered_map<std::string, std::vector<uint8_t>> patternsToJMPFromJNZ {
    {"AchievementGui::allowed", {0x75, 0x86, 0x80, 0xbb, 0xef, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x85, 0x79, 0xff, 0xff, 0xff}}
};

std::unordered_map<std::string, std::vector<uint8_t>> patternsToJMP {
    {"SteamContext::setStat", {0x74, 0x2d, 0x0f, 0x1f, 0x40, 0x00, 0x48, 0x8b, 0x10, 0x80, 0x7a, 0x3e, 0x00, 0x74, 0x17, 0x80, 0x7a, 0x40, 0x00, 0x74, 0x11, 0x80, 0x7a}},
    {"SteamContext::unlockAchievement", {0x74, 0x29, 0x48, 0x8b, 0x10, 0x80, 0x7a, 0x3e, 0x00, 0x74, 0x17}}
};

std::unordered_map<std::string, std::vector<uint8_t>> patternsToCMOVZ {
    {"PlayerData::PlayerData", {0x45, 0xf0, 0xe8, 0x12, 0xe7, 0x2f, 0x01}}
};

//Usage Example: ./FAE_Linux /home/senpaii/steamdrives/nvme1/SteamLibrary/steamapps/common/Factorio/bin/x64/factorio

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
    for (auto &patternPair: patternsToJNZ) {
        doPatching(factorioFilePath, patternPair, 0);
    }

    Log::LogF("Patching instructions to JMP from JZ..\n");
    for (auto &patternPair: patternsToJMP) {
        doPatching(factorioFilePath, patternPair, 1);
    }

    Log::LogF("Patching instructions to JMP from JNZ..\n");
    for (auto &patternPair: patternsToJMPFromJNZ) {
        doPatching(factorioFilePath, patternPair, 2);
    }

    Log::LogF("Patching instructions to CMOVZ from CMOVNZ..\n");
    for (auto &patternPair : patternsToCMOVZ) {
        doPatching(factorioFilePath, patternPair, 3);
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