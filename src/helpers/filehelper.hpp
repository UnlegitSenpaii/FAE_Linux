#pragma once

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <vector>

namespace FileHelper {
    bool DoesFileExist(const std::string& name);
    bool MarkFileExecutable(const std::string& file_path);
    bool ReadFileToBuffer(const std::string& filePath, std::vector<std::uint8_t>& buffer);
    bool WriteBufferToFile(const std::string& filePath, const std::vector<std::uint8_t>& buffer);
}