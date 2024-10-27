#ifndef FAE_LINUX_FILEHELPER_HPP
#define FAE_LINUX_FILEHELPER_HPP

#include <cstdint>
#include <filesystem>
#include <vector>

namespace FileHelper
{
    std::vector<std::uint8_t> read(const std::filesystem::path& path);
    void write(const std::filesystem::path& path, const std::vector<std::uint8_t>& data);
    bool MarkFileExecutable(const std::filesystem::path& path);
} // FileHelper

#endif //FAE_LINUX_FILEHELPER_HPP
