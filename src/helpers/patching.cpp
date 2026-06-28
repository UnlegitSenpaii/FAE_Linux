#include "patching.hpp"
#include "logging.hpp"

std::uint8_t HexCharToValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    throw std::invalid_argument("Invalid hex character");
}

std::uint8_t HexToByte(const std::string& hex)
{
    if (hex.size() != 2)
        throw std::invalid_argument("Invalid hex string length");
    return (HexCharToValue(hex[0]) << 4) | HexCharToValue(hex[1]);
}

bool MatchesPattern(const std::vector<std::uint8_t>& buffer, const std::vector<std::optional<std::uint8_t>>& pattern, std::size_t pos)
{
    for (std::size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i].has_value() && buffer[pos + i] != pattern[i].value()) {
            return false;
        }
    }
    return true;
}

std::vector<std::vector<std::optional<std::uint8_t>>> Patcher::ParsePattern(std::string_view input)
{
    std::vector<std::vector<std::optional<std::uint8_t>>> result;
    std::vector<std::optional<std::uint8_t>> current;

    size_t i = 0;
    while (i < input.size()) {
        char c = input[i];

        if (c == ' ' || c == '\t') {
            ++i;
            continue;
        }

        if (c == '?') {
            current.push_back(std::nullopt);
            ++i;
            continue;
        }

        if (c == '*') {
            if (!current.empty()) {
                result.push_back(std::move(current));
                current.clear();
            }
            ++i;
            continue;
        }

        if (i + 1 < input.size()) {
            std::uint8_t hi = HexCharToValue(input[i]);
            std::uint8_t lo = HexCharToValue(input[i + 1]);
            current.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
            i += 2;
            continue;
        }

        throw std::invalid_argument("Incomplete hex byte in pattern");
    }

    if (!current.empty()) {
        result.push_back(std::move(current));
    }

    return result;
}

bool Patcher::GenerateFlexPattern(const std::vector<uint8_t>& buffer, const std::vector<std::vector<std::optional<std::uint8_t>>>& parts, size_t startPos, size_t& matchEnd)
{
    size_t cursor = startPos;
    for (size_t p = 0; p < parts.size(); ++p) {
        const auto& part = parts[p];
        if (cursor + part.size() > buffer.size())
            return false;

        if (MatchesPattern(buffer, part, cursor)) {
            cursor += part.size();
            continue;
        }

        if (p == 0)
            return false;

        size_t searchEnd = std::min(buffer.size() - part.size() + 1, cursor + Patcher::FLEX_GAP_MAX + 1);
        bool found = false;

        for (size_t scan = cursor; scan < searchEnd; ++scan) {
            if (MatchesPattern(buffer, part, scan)) {
                cursor = scan + part.size();
                found = true;
                break;
            }
        }

        if (!found)
            return false;
    }
    matchEnd = cursor;
    return true;
}

bool Patcher::GenerateSearchPattern(const std::vector<std::uint8_t>& buffer, const std::string& incompleteSearchPattern, std::vector<std::uint8_t>& searchPattern)
{
    std::vector<std::vector<std::optional<std::uint8_t>>> pattern;
    try {
        pattern = ParsePattern(incompleteSearchPattern);
    } catch (const std::invalid_argument& e) {
        Log::LogF("Error in Pattern: %s\n", e.what());
        return false;
    }

    if (pattern.empty() || pattern[0].empty())
        return false;

    if (pattern.size() > 1) {
        const auto& firstSegment = pattern[0];
        for (size_t pos = 0; pos <= buffer.size() - firstSegment.size(); ++pos) {
            if (!MatchesPattern(buffer, firstSegment, pos))
                continue;

            size_t matchEnd;
            if (!GenerateFlexPattern(buffer, pattern, pos, matchEnd))
                continue;

            searchPattern.insert(searchPattern.end(),
                buffer.begin() + pos, buffer.begin() + matchEnd);
            return true;
        }
        return false;
    }

    const auto& segment = pattern[0];

    bool allConcrete = true;
    for (const auto& atom : segment) {
        if (!atom.has_value()) {
            allConcrete = false;
            break;
        }
    }

    if (allConcrete) {
        searchPattern.resize(segment.size());
        for (std::size_t i = 0; i < segment.size(); ++i)
            searchPattern[i] = segment[i].value();
        return true;
    }

    for (std::size_t bufferPos = 0; bufferPos <= buffer.size() - segment.size(); ++bufferPos) {
        if (!MatchesPattern(buffer, segment, bufferPos))
            continue;

        searchPattern.insert(searchPattern.end(), buffer.begin() + bufferPos, buffer.begin() + bufferPos + segment.size());
        return true;
    }

    return false;
}

bool Patcher::ReplaceHexPattern(std::vector<std::uint8_t>& buffer, const std::vector<std::uint8_t>& searchPattern, const std::vector<std::uint8_t>& replacePattern, int expectedPatchCount)
{
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

std::vector<std::uint8_t> Patcher::GenerateReplacePattern(const std::vector<std::uint8_t>& searchPattern, int replaceInstruction)
{
    if (searchPattern.size() < 2)
        return { };

    std::vector<std::uint8_t> replacePattern;

    switch (replaceInstruction) {
    case PATCH_TYPE_JZJNZ: // JZ -> JNZ (0x75 or 0x85) instruction
        if (searchPattern[0] != 0x74 && searchPattern[0] != 0x84) // Check for JZ (0x74 or 0x84)
            return { };
        replacePattern.push_back(searchPattern[0] == 0x74 ? 0x75 : 0x85);
        break;

    case PATCH_TYPE_JZJMP: // JZ -> JMP (0xEB) instruction
        if (searchPattern[0] != 0x74 && searchPattern[0] != 0x84) // Check for JZ
            return { };
        replacePattern.push_back(0xEB);
        break;

    case PATCH_TYPE_JNZJMP: // JNZ (0F 85)-> JMP ~ huh? im surprised this doesn't break stuff -- it did indeed break stuff
        if (searchPattern[0] != 0x0F && searchPattern[1] != 0x85 && // Check for JNZ (0F 85)
            searchPattern[0] != 0x75) // Check for JNZ SHORT (75)
            return { };
        replacePattern.push_back(0xEB);
        break;

    case PATCH_TYPE_JNZJZ: // JNZ -> JZ (0x74 or 0x84) instruction
        if (searchPattern[0] != 0x0F && searchPattern[1] != 0x85 && // Check for JNZ (0F 85)
            searchPattern[0] != 0x75) // Check for JNZ SHORT (75)
            return { };
        replacePattern.push_back(searchPattern[0] == 0x75 ? 0x74 : 0x84);
        break;

    case PATCH_TYPE_CMOVNZCMOVZ: // CMOVNZ (0F 45) -> CMOVZ (0F 44) -- Prefix (0f) discarded!!
        if (searchPattern[0] != 0x45) // Check for CMOVNZ (0F 45) -- Prefix (0f) discarded!!
            return { };
        replacePattern.push_back(0x44);
        break;

    default:
        Log::LogF("Error: Patch Instruction Mode '%d' not implemented yet!\n", replaceInstruction);
        return { };
    }

    replacePattern.insert(replacePattern.end(), searchPattern.begin() + replacePattern.size(), searchPattern.end());
    return replacePattern;
}
