#ifndef FAE_LINUX_FILEHELPER_HPP
#define FAE_LINUX_FILEHELPER_HPP

#include <string>

namespace FileHelper
{
    bool DoesFileExist(const std::string& name);

    bool MarkFileExecutable(const std::string& file_path);
} // FileHelper

#endif //FAE_LINUX_FILEHELPER_HPP
