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
 * 74 6e 0f 1f 44 00 00 48 8b 10
 *
 * AchievementGui::AchievementGui jz > jnz
 * 84 d8 04 00 00 0f 1f 00
 *
 * ModManager::isVanilla // modifying this directly might break stuff
 * -> this function is used in the linux version in PlayerData to determine if the game is modded or not.
 * In Windows, it's not (or the compiler optimizes it out)
 *
 * PlayerData::PlayerData CMOVNZ > CMOVZ -- Prefix (0f) discarded!!
 * 45 f0 e8 bf a8 43 01
 *
 * SteamContext::setStat jz > jmp
 * 74 2b 66 90 48 8b 10 80 7a 3e 00 74 17
 *
 * SteamContext::unlockAchievement jz > jmp
 * 74 29 48 8b 10 80 7a 3e 00 74 17
 *
 * AchievementGui::allowed (map) jnz > jmp
 * 0f 85 7a ff ff ff 48 8b bb d0 03 00 00 b8 01
 */

std::unordered_map<std::string, std::vector<uint8_t>> patternsToJNZ{
        {"SteamContext::onUserStatsReceived", {0x74, 0x6e, 0x0f, 0x1f, 0x44, 0x00, 0x00, 0x48, 0x8b, 0x10}},
        {"AchievementGui::AchievementGui",    {0x84, 0xd8, 0x04, 0x00, 0x00, 0x0f, 0x1f, 0x00}}
};

std::unordered_map<std::string, std::vector<uint8_t>> patternsToJMPFromJNZ{
        {"AchievementGui::allowed", {0x0f, 0x85, 0x7a, 0xff, 0xff, 0xff, 0x48, 0x8b, 0xbb, 0xd0, 0x03, 0x00, 0x00, 0xb8, 0x01}}
};

std::unordered_map<std::string, std::vector<uint8_t>> patternsToJMP{
        {"SteamContext::setStat",           {0x74, 0x2b, 0x66, 0x90, 0x48, 0x8b, 0x10, 0x80, 0x7a, 0x3e, 0x00, 0x74, 0x17}},
        {"SteamContext::unlockAchievement", {0x74, 0x29, 0x48, 0x8b, 0x10, 0x80, 0x7a, 0x3e, 0x00, 0x74, 0x17}}
};

std::unordered_map<std::string, std::vector<uint8_t>> patternsToCMOVZ{
        {"PlayerData::PlayerData", {0x45, 0xf0, 0xe8, 0xbf, 0xa8, 0x43, 0x01}}
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
    for (auto &patternPair: patternsToCMOVZ) {
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