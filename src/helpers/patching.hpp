#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <cerrno>
#include <cstring>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <algorithm>

enum PATCH_TYPE
{
    PATCH_TYPE_JZJNZ,
    PATCH_TYPE_JZJMP,
    PATCH_TYPE_JNZJMP,
    PATCH_TYPE_JNZJZ,
    PATCH_TYPE_CMOVNZCMOVZ,
    PATCH_TYPE_MAX
};

struct patternData_t
{
    PATCH_TYPE patchType;
    std::string patternName;
    std::string pattern;
    int expectedPatchCount = 1;
    bool optional = false;
};

namespace Patcher
{


	bool GenerateSearchPattern(const std::vector<std::uint8_t> &buffer, const std::string &incompleteSearchPattern, std::vector<std::uint8_t> &searchPattern);
	bool ReplaceHexPattern(std::vector<std::uint8_t> &buffer, const std::vector<std::uint8_t> &searchPattern, const std::vector<std::uint8_t> &replacePattern, int expectedPatchCount = 1);
	std::vector<std::uint8_t> GenerateReplacePattern(const std::vector<std::uint8_t>& searchPattern, int replaceInstruction);
    bool FindPattern(const std::vector<std::uint8_t>& buffer, const std::string& patternAsString, std::vector<std::size_t>& matchPositions);
}
