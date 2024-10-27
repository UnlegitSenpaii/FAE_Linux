#pragma once

#include <cstdint>
#include <vector>

namespace Patcher
{
    constexpr std::size_t kBufferSize = 1024;

    bool ReplaceHexPattern(
        std::vector<std::uint8_t>& data,
        const std::vector<std::uint8_t>& searchPattern,
        const std::vector<std::uint8_t>& replacePattern);

    std::vector<std::uint8_t> GenerateReplacePattern(
        const std::vector<std::uint8_t>& searchPattern,
        int replaceInstruction);

} // Patcher