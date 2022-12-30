#ifndef FACTORIOACHIEVEMENTENABLER_LINUX_LOGGING_HPP
#define FACTORIOACHIEVEMENTENABLER_LINUX_LOGGING_HPP

#include <cstdarg>
#include <string>
#include <fstream>

namespace Log
{
	static std::string LogFileName;
	static bool SaveToFile = false;

	void Initialize(const std::string&, bool);

	void PrintSmugAstolfo();

	void PrintAsciiArtWelcome();

	void LogF(const char* fmt, ...);
}


#endif //FACTORIOACHIEVEMENTENABLER_LINUX_LOGGING_HPP
