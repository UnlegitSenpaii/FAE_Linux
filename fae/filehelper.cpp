#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <fae/filehelper.hpp>

namespace FileHelper
{
    std::vector<std::uint8_t> read(const std::filesystem::path& path)
    {
        std::vector<std::uint8_t> data;
        data.resize(std::filesystem::file_size(path));

        std::ifstream file{path, std::ios::binary};
        file.read(reinterpret_cast<char*>(&data[0]), data.size());
        std::cout << "DEBUG: File size: " << data.size() << std::endl;
        return data;
    }

    void write(const std::filesystem::path& path, const std::vector<std::uint8_t>& data)
    {
        std::ofstream file{path, std::ios::binary | std::ios::trunc};
        file.write(reinterpret_cast<const char*>(&data[0]), data.size());
    }

    bool MarkFileExecutable(const std::filesystem::path& path)
    {
        if (chmod(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
            std::cerr << "[FAE]" << "Error marking file " << path << " as executable: " << std::strerror(errno) << std::endl;
            return false;
        }
        return true;
    }
}
