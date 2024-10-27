#include <algorithm>
#include <iostream>
#include <fae/patching.hpp>

bool Patcher::ReplaceHexPattern(
    std::vector<std::uint8_t>& data,
    const std::vector<std::uint8_t>& searchPattern,
    const std::vector<std::uint8_t>& replacePattern)
{
    if (searchPattern.size() != replacePattern.size()) {
        std::cerr << "[FAE] Error: Search and replace patterns must be the same size." << std::endl;
        return false;
    }

    int matchesFound = 0;
    std::vector<std::uint8_t>::iterator found = std::begin(data);

    while ((found = std::search(found, std::end(data), std::begin(searchPattern), std::end(searchPattern))) < std::end(data)) {
        std::copy(std::begin(replacePattern), std::end(replacePattern), found);
        found += replacePattern.size();
        ++matchesFound;
    }

    std::cout << " " << matchesFound << " matches for the pattern." << std::endl;

    return matchesFound > 0;
}

std::vector<std::uint8_t> Patcher::GenerateReplacePattern(
    const std::vector<std::uint8_t>& searchPattern,
    int replaceInstruction)
{
    if (searchPattern.size() < 2)
        return {};

    std::vector<std::uint8_t> replacePattern;

    //do I get paid for writing this in a clean way? no. do I get paid at all? no. :(
    switch (replaceInstruction) {
        case 0://JZ -> JNZ (0x75 or 0x85) instruction
            if (searchPattern[0] != 0x74 && searchPattern[0] != 0x84) //Check for JZ (0x74 or 0x84)
                return {};
            replacePattern.push_back(searchPattern[0] == 0x74 ? 0x75 : 0x85);
            break;

        case 1://JZ -> JMP (0xEB) instruction
            if (searchPattern[0] != 0x74 && searchPattern[0] != 0x84) //Check for JZ
                return {};
            replacePattern.push_back(0xEB);
            break;

        case 2:// JNZ (0F 85)-> JMP ~ huh? im surprised this doesn't break stuff -- it did indeed break stuff
            if (searchPattern[0] != 0x0F && searchPattern[1] != 0x85 && // Check for JNZ (0F 85)
                searchPattern[0] != 0x75)                               // Check for JNZ SHORT (75)
                return {};
            replacePattern.push_back(0xEB);
            break;

        case 3://CMOVNZ (0F 45) -> CMOVZ (0F 44) -- Prefix (0f) discarded!!
            if (searchPattern[0] != 0x45) //Check for CMOVNZ (0F 45) -- Prefix (0f) discarded!!
                return {};
            replacePattern.push_back(0x44);
            break;

        default:
            std::cerr << "[FAE]" << "Error: Patch Instruction Mode '" << replaceInstruction << " not implemented yet!" << std::endl;
            return {};
    }

    replacePattern.insert(replacePattern.end(), searchPattern.begin() + replacePattern.size(), searchPattern.end());
    return replacePattern;
}
