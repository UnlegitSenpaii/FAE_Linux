#include "patching.hpp"
#include "logging.hpp"

std::uint8_t HexCharToValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    throw std::invalid_argument("Invalid hex character");
}

std::uint8_t HexToByte(const std::string &hex) {
    if (hex.size() != 2) throw std::invalid_argument("Invalid hex string length");
    return (HexCharToValue(hex[0]) << 4) | HexCharToValue(hex[1]);
}

std::vector<std::optional<std::uint8_t>> ConvertPattern(const std::string &patternAsString) {
    std::vector<std::optional<std::uint8_t>> pattern;
    std::istringstream stream(patternAsString);
    std::string byteStr;

    while (stream >> byteStr) {
        if (byteStr == "?") {
            pattern.push_back(std::nullopt);
            continue;
        }

        pattern.push_back(HexToByte(byteStr));    
    }
    return pattern;
}

//optional magic: will not contain a value in the location if the byte is a wildcard
bool MatchesPattern(const std::vector<std::uint8_t> &buffer, const std::vector<std::optional<std::uint8_t>> &pattern, std::size_t pos) {
    for (std::size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i].has_value() && buffer[pos + i] != pattern[i].value()) {
            return false;
        }
    }
    return true;
}

bool Patcher::GenerateSearchPattern(const std::vector<std::uint8_t> &buffer, const std::string &incompleteSearchPattern, std::vector<std::uint8_t> &searchPattern) {
    std::vector<std::optional<std::uint8_t>> pattern{};

    try {
        pattern = ConvertPattern(incompleteSearchPattern);
    } catch (const std::invalid_argument &e) {
        Log::LogF("Error in Pattern: %s\n", e.what());
        return false;
    }

    //check if we have wildcards, if not, we can just return the pattern
    if (std::none_of(pattern.begin(), pattern.end(), [](const std::optional<std::uint8_t> &byte) { return !byte.has_value(); })) {
        searchPattern = std::vector<std::uint8_t>(pattern.size());
        for (std::size_t i = 0; i < pattern.size(); ++i) {
            searchPattern[i] = pattern[i].value();
        }
        return true;
    }


    for (std::size_t bufferPos = 0; bufferPos <= buffer.size() - pattern.size(); ++bufferPos) {
        if (!MatchesPattern(buffer, pattern, bufferPos)) {
            continue;
        }

        searchPattern.insert(searchPattern.end(), buffer.begin() + bufferPos, buffer.begin() + bufferPos + pattern.size());
        return true;
    }

    return false;
}

bool Patcher::ReplaceHexPattern(std::vector<std::uint8_t> &buffer, const std::vector<std::uint8_t> &searchPattern, const std::vector<std::uint8_t> &replacePattern, int expectedPatchCount) {
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
    
    Log::LogF(" %d matches for the pattern, expected patches: %d.\n", matchesFound, expectedPatchCount);
    return matchesFound == expectedPatchCount;
}


std::vector<std::uint8_t> Patcher::GenerateReplacePattern(const std::vector<std::uint8_t> &searchPattern, int replaceInstruction) {
    if (searchPattern.size() < 2)
        return {};

    std::vector<std::uint8_t> replacePattern;

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

        case PATCH_TYPE_JNZJZ://JNZ -> JZ (0x74 or 0x84) instruction
            if (searchPattern[0] != 0x0F && searchPattern[1] != 0x85 && // Check for JNZ (0F 85)
                searchPattern[0] != 0x75)                               // Check for JNZ SHORT (75)
                return {};
            replacePattern.push_back(searchPattern[0] == 0x75 ? 0x74 : 0x84);
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