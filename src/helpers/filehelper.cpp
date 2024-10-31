#include "filehelper.hpp"

bool FileHelper::DoesFileExist(const std::string& name)
{
	struct stat buffer{};
	return (stat(name.c_str(), &buffer) == 0);
}

bool FileHelper::MarkFileExecutable(const std::string& file_path)
{
	if (chmod(file_path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) != 0)
	{
		std::cerr << "[FAE]" << "Error marking file '" << file_path << "' as executable: " << std::strerror(errno) << std::endl;
		return false;
	}
	return true;
}


bool FileHelper::ReadFileToBuffer(const std::string &filePath, std::vector<std::uint8_t> &buffer) {
    std::ifstream inputFile(filePath, std::ios::binary);
    if (!inputFile.is_open()) 
        return false;

    buffer = std::vector<std::uint8_t>((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();
    return true;
}

bool FileHelper::WriteBufferToFile(const std::string &filePath, const std::vector<std::uint8_t> &buffer) {
    std::ofstream outputFile(filePath, std::ios::binary);
    if (!outputFile.is_open()) 
        return false;
    
    outputFile.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());
    outputFile.close();
    return true;
}