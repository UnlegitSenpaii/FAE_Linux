#include <iostream>
#include <vector>
#include <unistd.h>
#include <unordered_map>
#include "helpers/logging.hpp"
#include "helpers/filehelper.hpp"
#include "helpers/patching.hpp"

/*
 * Want to update the patterns yourself?
 * check out the wiki: https://github.com/UnlegitSenpaii/FAE_Linux/wiki/Finding-the-currently-used-patterns-in-ghidra
 * 
 * why is ghidra 11.3.1 so ass?
 * Patterns:
 * (missing -> turned to part 1 of OnUserStatsReceived) SteamContext::unlockAchievementsThatAreOnSteamButArentActivatedLocally jz > jnz
 * 74 37 66 0f 1f 44 00 00 48 8b 10 80
 * if this one breaks, look at the first match for 74 ? 66 0F 1F 44 ? ? 48 8B ? 80 7A ? 00
 * 
 * (missing  -> turned to part 2 of OnUserStatsReceived) SteamContext::updateAchievementStatsFromSteam jz > jnz
 * 74 30 0f 1f 80 00 00 00 00 48 8b 10
 *
 * AchievementGui::refresh ---- XREF gui-achievements.modded-game
 * 75 ea 80 7a 40 01 75 e4 80 7a 41 01 74 de -- JNZ > JZ
 *
 * note for me: this is the achievements.dat & achievements-modded.dat thingy
 * todo: instead of doing this, just edit achievements-modded.dat to achievements.dat 
 * PlayerData::PlayerData JZ > jnz
 * 74 2e 48 8d 15 2b 91 e6 fd eb 0d 0f 1f 40 00 48 83 c0 08 
 *
 * SteamContext::setStat jz > jmp
 * 74 1a 4c 8b 00 41 80 78 3e 01 75 ed 41 80 78 40 01 75 e6 41 80 78 41
 *
 * SteamContext::unlockAchievement jz > jmp 
 * 74 17 48 8b 10 80 7a 3e 01 75 ee 80 7a 40 01 75 e8 80 7a 41 01 74 e2 eb 3c
 * 
 * SteamContext::OnUserStatsReceived JZ > JMP
 * 74 ?  48 8b ?  80 7A ?  ?  ?  ?  80 7A ?  ?  ?  ?  80 7A ?  ?  ?  ?  e9 22 01 00 00
 * 74 68 48 ba 74 65 73 74 5f 6d 6f 64 eb 0f 66 0f 1f 44 00 00 48 83 c0 08 
 * 
 * AchievementGui::allowed (map) jz > jmp
 * 74 07 48 83 78 20 00 75 cc   JZ > JNZ    //maybe not needed
 * 75 cc 49 8b 80 ? 01     JNZ > JMP 
 */

std::vector<patternData_t> patternList = {
    /*
        JZ -> JNZ Patches
    */
    {PATCH_TYPE_JZJNZ, "PlayerData::PlayerData", 
    "74 2e 48 8d 15 ? ? ? ? eb 0d 0f 1f 40 00 48 83 c0 08", 1, true},

    /*
        JZ -> JMP Patches
    */
    {PATCH_TYPE_JZJMP, "SteamContext::setStat", 
    "74 1a 4c 8b 00 41 80 78 3e 01 75 ed 41 80 78 40 01 75 e6 41 80 78 41"},

    {PATCH_TYPE_JZJMP, "SteamContext::unlockAchievement", 
    "74 17 48 8b 10 80 7a 3e 01 75 ee 80 7a 40 01 75 e8 80 7a 41 01 74 e2 eb 3c"},

    {PATCH_TYPE_JZJMP, "SteamContext::OnUserStatsReceived", 
    "74 ? 48 8b ? 80 7A ? ? ? ? 80 7A ? ? ? ? 80 7A ? ? ? ? e9 22 01 00 00"},

    {PATCH_TYPE_JZJMP, "SteamContext::OnUserStatsReceived2", 
    "74 68 48 ba 74 65 73 74 5f 6d 6f 64 eb 0f 66 0f 1f 44 00 00 48 83 c0 08"},

    {PATCH_TYPE_JZJMP, "AchievementGui::allowed",
    "74 07 48 83 78 20 00 75 cc"},

    /*
        JNZ -> JZ Patches
    */

    {PATCH_TYPE_JNZJZ, "AchievementGui::refresh",
    "74 de 48 8d 0d b6 2c 34 ff"},

    /*
        JNZ -> JMP Patches
    */
    {PATCH_TYPE_JNZJMP, "AchievementGui::allowed2", 
    "75 cc 49 8b 80 ? 01"},

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
