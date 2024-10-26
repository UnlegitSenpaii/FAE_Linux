#ifndef FAE_LINUX_PATCHING_HPP
#define FAE_LINUX_PATCHING_HPP

#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <cerrno>
#include <cstring>

namespace Patcher
{
    constexpr std::size_t kBufferSize = 1024;

    bool ReplaceHexPattern(
        const std::string& file_path,
        const std::vector<std::uint8_t>& searchPattern,
        const std::vector<std::uint8_t>& replacePattern);

    std::vector<std::uint8_t> GenerateReplacePattern(
        const std::vector<std::uint8_t>& searchPattern,
        int replaceInstruction);

} // Patcher

#endif //FAE_LINUX_PATCHING_HPP
