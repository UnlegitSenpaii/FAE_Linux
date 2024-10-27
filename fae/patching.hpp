#pragma once

#include <cstdint>
#include <vector>

namespace Patcher
{
    bool ReplaceHexPattern(
        std::vector<std::uint8_t>& data,
        const std::vector<std::uint8_t>& searchPattern,
        const std::vector<std::uint8_t>& replacePattern);

    std::vector<std::uint8_t> GenerateReplacePattern(
        const std::vector<std::uint8_t>& searchPattern,
        int replaceInstruction);

} // Patcher
