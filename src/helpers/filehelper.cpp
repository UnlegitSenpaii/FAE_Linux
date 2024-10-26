#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/stat.h>
#include "filehelper.hpp"

bool FileHelper::DoesFileExist(const std::string& name)
{
    struct stat buffer{};
    return (stat(name.c_str(), &buffer) == 0);
}

bool FileHelper::MarkFileExecutable(const std::string& file_path)
{
    if (chmod(file_path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
        std::cerr << "[FAE]" << "Error marking file '" << file_path << "' as executable: " << std::strerror(errno) << std::endl;
        return false;
    }
    return true;
}
