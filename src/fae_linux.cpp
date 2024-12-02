#include <iostream>
#include <vector>
#include <unistd.h>
#include <unordered_map>
#include "helpers/logging.hpp"
#include "helpers/filehelper.hpp"
#include "helpers/patching.hpp"

/*
 * Patterns:
 * SteamContext::unlockAchievementsThatAreOnSteamButArentActivatedLocally jz > jnz
 * 74 37 66 0f 1f 44 00 00 48 8b 10 80
 * if this one breaks, try 74 37 66 0F 1F 44 00 00 48 8B ?? 80 7A ?? 00
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
 * 45 f0 e8 b9 51 f4 fe          CMOVNZ > CMOVZ
 * 
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

std::vector<patternData_t> patternList = {
    /*
        JZ -> JNZ Patches
    */
    {PATCH_TYPE_JZJNZ, "SteamContext::unlockAchievementsThatAreOnSteamButArentActivatedLocally",
    "74 37 66 0f 1f 44 00 00 48 8b 10 80"},

    {PATCH_TYPE_JZJNZ, "SteamContext::updateAchievementStatsFromSteam", 
    "74 30 0f 1f 80 00 00 00 00 48 8b 10"},

    /*
        JNZ -> JMP Patches
    */
    {PATCH_TYPE_JNZJMP, "AchievementGui::updateModdedLabel",
    "75 5b 55 45 31 C0 45 31"},

    /*
        JZ -> JMP Patches
    */
    {PATCH_TYPE_JZJMP, "SteamContext::setStat & SteamContext::unlockAchievement", 
    "74 2d ? ? ? ? 48 8b 10 80 7a 3e 00 74 17 80 7a 40 00 74 11 80 7a", 2},

    {PATCH_TYPE_JZJMP, "AchievementGui::allowed",
    "74 17 48 83 78 20 00"},

    {PATCH_TYPE_JZJMP, "AchievementGui::allowed2", 
    "74 10 31 c0 5b 41 5c 41 5d 41 5e"},

    /*
        CMOVNZ -> CMOVZ Patches
    */
    {PATCH_TYPE_CMOVNZCMOVZ, "PlayerData::PlayerData", 
    "45 f0 e8 ? ? ? ? 48 8b 05 ? ? ? ? 49 8d bd ? ? ? ? 48 89 da 48 89 bd", 1, true},
};

void doPatching(std::vector<std::uint8_t> &buffer, const patternData_t& patternData) {
    Log::LogF("Patching %s:\n", patternData.patternName.c_str());
    
    std::vector<std::uint8_t> completeSearchPattern;
    if (!Patcher::GenerateSearchPattern(buffer, patternData.pattern, completeSearchPattern)) {
        Log::LogF(" -> FAILED!\n");
        return;
    }
    
    const std::vector<uint8_t> replacementPattern = Patcher::GenerateReplacePattern(completeSearchPattern, patternData.patchType);
    
    std::string completePatternAsString;
    for (const auto &byte : completeSearchPattern) {
        char buffer[4]; //just dont overflow, please
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


int main(int argc, char *argv[]) {
    Log::PrintAsciiArtWelcome();
    Log::Initialize("./FAE_Debug.log", false);
    Log::LogF("Initialized Logging.\n");

    if (argc < 2) {
        Log::LogF("Incorrect Usage!\nUsage: %s [Factorio File Path]\n", argv[0]);
        return 1;
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

    for (const auto &patternEntry : patternList) {
        if(!patternEntry.optional){
            doPatching(buffer, patternEntry);
            continue;
        }

        std::string userInput = "";
        Log::LogF("\033[1mDo you want to use the modded achievement save? (y/N)\033[0m\n");
        std::getline(std::cin, userInput);

        if (userInput.empty() || std::tolower(userInput[0]) == 'n') {
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
