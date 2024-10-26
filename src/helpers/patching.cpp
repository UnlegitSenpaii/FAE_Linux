#include "patching.hpp"

bool Patcher::ReplaceHexPattern(
    const std::string& filePath,
    const std::vector<std::uint8_t>& searchPattern,
    const std::vector<std::uint8_t>& replacePattern)
{
    if (searchPattern.size() != replacePattern.size()) {
        std::cerr << "[FAE]" << "Error: Search and replace patterns must be the same size." << std::endl;
        return false;
    }

    std::ifstream inputFile(filePath, std::ios::binary);
    if (!inputFile.is_open()) {
        std::cerr << "[FAE]" << "Error: Unable to open file '" << filePath << "' for reading." << std::endl;
        return false;
    }

    std::string tempFilePath = filePath + ".tmp";
    std::ofstream outputFile(tempFilePath, std::ios::binary);
    if (!outputFile.is_open()) {
        std::cerr << "[FAE]" << "Error: Unable to open temporary file '" << tempFilePath << "' for writing." << std::endl;
        return false;
    }

    std::vector<std::uint8_t> buffer(kBufferSize);
    int matchesFound = 0;

    while (inputFile.read(reinterpret_cast<char*>(buffer.data()), kBufferSize)) {
        std::size_t bufferPos = 0;
        while (bufferPos < kBufferSize) {
            if (buffer[bufferPos] != searchPattern[0]) {
                outputFile.put(buffer[bufferPos]);
                ++bufferPos;
                continue;
            }

            bool match = true;
            for (std::size_t i = 0; i < searchPattern.size(); ++i) {
                if (buffer[bufferPos + i] != searchPattern[i])
                {
                    match = false;
                    break;
                }
            }

            if (!match) {
                outputFile.put(buffer[bufferPos]);
                ++bufferPos;
                continue;
            }

            outputFile.write(reinterpret_cast<const char*>(replacePattern.data()), replacePattern.size());
            bufferPos += searchPattern.size();
            ++matchesFound;
        }
    }

    inputFile.close();
    outputFile.close();

    if (std::rename(tempFilePath.c_str(), filePath.c_str()) != 0) {
        std::cerr << "[FAE]" << "Error: Unable to replace file '" << filePath << "' with temporary file '" << tempFilePath << "'." << std::endl;
        return false;
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
