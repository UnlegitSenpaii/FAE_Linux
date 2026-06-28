#pragma once

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

enum PATCH_TYPE {
    PATCH_TYPE_JZJNZ,
    PATCH_TYPE_JZJMP,
    PATCH_TYPE_JNZJMP,
    PATCH_TYPE_JNZJZ,
    PATCH_TYPE_CMOVNZCMOVZ,
    PATCH_TYPE_MAX
};

struct patternData_t {
    PATCH_TYPE patchType;
    std::string patternName;
    std::string pattern;
    int expectedPatchCount = 1;
    bool optional = false;
};

namespace Patcher {
    static constexpr size_t FLEX_GAP_MAX = 16;

    std::vector<std::vector<std::optional<std::uint8_t>>> ParsePattern(std::string_view input);
    bool GenerateSearchPattern(const std::vector<std::uint8_t>& buffer, const std::string& incompleteSearchPattern, std::vector<std::uint8_t>& searchPattern);
    bool GenerateFlexPattern(const std::vector<uint8_t>& buffer, const std::vector<std::vector<std::optional<std::uint8_t>>>& parts, size_t startPos, size_t& matchEnd);
    bool ReplaceHexPattern(std::vector<std::uint8_t>& buffer, const std::vector<std::uint8_t>& searchPattern, const std::vector<std::uint8_t>& replacePattern, int expectedPatchCount = 1);
    std::vector<std::uint8_t> GenerateReplacePattern(const std::vector<std::uint8_t>& searchPattern, int replaceInstruction);
}
