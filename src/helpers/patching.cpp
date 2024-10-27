#include "patching.hpp"
#include "logging.hpp"

bool Patcher::ReadFileToBuffer(const std::string &filePath, std::vector<std::uint8_t> &buffer) {
    std::ifstream inputFile(filePath, std::ios::binary);
    if (!inputFile.is_open()) 
        return false;

    buffer = std::vector<std::uint8_t>((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();
    return true;
}

bool Patcher::WriteBufferToFile(const std::string &filePath, const std::vector<std::uint8_t> &buffer) {
    std::ofstream outputFile(filePath, std::ios::binary);
    if (!outputFile.is_open()) 
        return false;
    
    outputFile.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());
    outputFile.close();
    return true;
}

bool Patcher::ReplaceHexPattern(std::vector<std::uint8_t> &buffer, const std::vector<std::uint8_t> &searchPattern, const std::vector<std::uint8_t> &replacePattern) {
    if (searchPattern.size() != replacePattern.size()) {
        Log::LogF("Error: Search and replace patterns must be the same size.\n");
        return false;
    }

    int matchesFound = 0;
    for (std::size_t bufferPos = 0; bufferPos <= buffer.size() - searchPattern.size(); ++bufferPos) {
        if (buffer[bufferPos] != searchPattern[0]) {
            continue;
        }

        bool match = true;
        for (std::size_t i = 0; i < searchPattern.size(); ++i) {
            if (buffer[bufferPos + i] != searchPattern[i]) {
                match = false;
                break;
            }
        }

        if (!match) {
            continue;
        }

        std::copy(replacePattern.begin(), replacePattern.end(), buffer.begin() + bufferPos);
        bufferPos += searchPattern.size() - 1;
        ++matchesFound;
    }
    
    Log::LogF(" %d matches for the pattern.\n", matchesFound);
    return matchesFound > 0;
}


std::vector<std::uint8_t> Patcher::GenerateReplacePattern(const std::vector<std::uint8_t> &searchPattern, int replaceInstruction) {
    if (searchPattern.size() < 2)
        return {};

    std::vector<std::uint8_t> replacePattern;

    //do I get paid for writing this in a clean way? no. do I get paid at all? no. :( 
    switch (replaceInstruction) {
        case PATCH_TYPE_JZJNZ://JZ -> JNZ (0x75 or 0x85) instruction
            if (searchPattern[0] != 0x74 && searchPattern[0] != 0x84) //Check for JZ (0x74 or 0x84)
                return {};
            replacePattern.push_back(searchPattern[0] == 0x74 ? 0x75 : 0x85);
            break;

        case PATCH_TYPE_JZJMP://JZ -> JMP (0xEB) instruction
            if (searchPattern[0] != 0x74 && searchPattern[0] != 0x84) //Check for JZ
                return {};
            replacePattern.push_back(0xEB);
            break;

        case PATCH_TYPE_JNZJMP:// JNZ (0F 85)-> JMP ~ huh? im surprised this doesn't break stuff -- it did indeed break stuff
            if (searchPattern[0] != 0x0F && searchPattern[1] != 0x85 && // Check for JNZ (0F 85)
                searchPattern[0] != 0x75)                               // Check for JNZ SHORT (75)
                return {};
            replacePattern.push_back(0xEB);
            break;

        case PATCH_TYPE_CMOVNZCMOVZ://CMOVNZ (0F 45) -> CMOVZ (0F 44) -- Prefix (0f) discarded!!
            if (searchPattern[0] != 0x45) //Check for CMOVNZ (0F 45) -- Prefix (0f) discarded!!
                return {};
            replacePattern.push_back(0x44);
            break;

        default:
            Log::LogF("Error: Patch Instruction Mode '%d' not implemented yet!\n", replaceInstruction);
            return {};
    }

    replacePattern.insert(replacePattern.end(), searchPattern.begin() + replacePattern.size(), searchPattern.end());
    return replacePattern;
}