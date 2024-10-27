#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <cerrno>
#include <cstring>

namespace Patcher
{
	enum PATCH_TYPE
	{
		PATCH_TYPE_JZJNZ = 0,
		PATCH_TYPE_JZJMP = 1,
		PATCH_TYPE_JNZJMP = 2,
		PATCH_TYPE_CMOVNZCMOVZ = 3,
		PATCH_TYPE_MAX
	};

	bool ReadFileToBuffer(const std::string& filePath, std::vector<std::uint8_t>& buffer);
	bool WriteBufferToFile(const std::string& filePath, const std::vector<std::uint8_t>& buffer);
	
	bool ReplaceHexPattern(std::vector<std::uint8_t>& buffer, const std::vector<std::uint8_t>& searchPattern, const std::vector<std::uint8_t>& replacePattern);

	std::vector<std::uint8_t> GenerateReplacePattern(const std::vector<std::uint8_t>& searchPattern, int replaceInstruction);
}