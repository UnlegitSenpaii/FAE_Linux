#include "patching.hpp"

bool Patcher::ReplaceHexPattern(const std::string& filePath, const std::vector<std::uint8_t>& searchPattern,
		const std::vector<std::uint8_t>& replacePattern)
{
	if (searchPattern.size() != replacePattern.size())
	{
		std::cerr << "[FAE]" << "Error: Search and replace patterns must be the same size." << std::endl;
		return false;
	}

	std::ifstream inputFile(filePath, std::ios::binary);
	if (!inputFile.is_open())
	{
		std::cerr << "[FAE]" << "Error: Unable to open file '" << filePath << "' for reading." << std::endl;
		return false;
	}

	std::string tempFilePath = filePath + ".tmp";
	std::ofstream outputFile(tempFilePath, std::ios::binary);
	if (!outputFile.is_open())
	{
		std::cerr << "[FAE]" << "Error: Unable to open temporary file '" << tempFilePath << "' for writing."
		          << std::endl;
		return false;
	}

	std::vector<std::uint8_t> buffer(kBufferSize);

	while (inputFile.read(reinterpret_cast<char*>(buffer.data()), kBufferSize))
	{
		std::size_t bufferPos = 0;
		while (bufferPos < kBufferSize)
		{
			if (buffer[bufferPos] != searchPattern[0])
			{
				outputFile.put(buffer[bufferPos]);
				++bufferPos;
				continue;
			}

			bool match = true;
			for (std::size_t i = 0; i < searchPattern.size(); ++i)
			{
				if (buffer[bufferPos + i] != searchPattern[i])
				{
					match = false;
					break;
				}
			}

			if (!match)
			{
				outputFile.put(buffer[bufferPos]);
				++bufferPos;
				continue;
			}

			outputFile.write(reinterpret_cast<const char*>(replacePattern.data()),
					replacePattern.size());
			bufferPos += searchPattern.size();
		}
	}

	inputFile.close();
	outputFile.close();

	if (std::rename(tempFilePath.c_str(), filePath.c_str()) != 0)
	{
		std::cerr << "[FAE]" << "Error: Unable to replace file '" << filePath << "' with temporary file '"
		          << tempFilePath
		          << "'." << std::endl;
		return false;
	}

	return true;
}


std::vector<std::uint8_t>
Patcher::GenerateReplacePattern(const std::vector<std::uint8_t>& searchPattern, bool doJNZInstruction)
{
	if (searchPattern.size() < 2)
		return {};

	std::vector<std::uint8_t> replacePattern;

	//Check for JZ instruction: (0x74 or 0x84)
	if (searchPattern[0] != 0x74 && searchPattern[0] != 0x84)
		return {};

	// Replace the JZ instruction with a JNZ (0x75 or 0x85) or JMP (0xEB) instruction
	if (doJNZInstruction)
		replacePattern.push_back(searchPattern[0] == 0x74 ? 0x75 : 0x85);
	else
		replacePattern.push_back(0xEB);

	replacePattern.insert(replacePattern.end(), searchPattern.begin() + 1, searchPattern.end());
	return replacePattern;
}