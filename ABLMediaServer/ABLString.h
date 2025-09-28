#pragma once
#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>

#include <cstdlib>
#ifdef _WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

namespace ABL {

	std::string& trim(std::string& s);

	// É¾³ý×Ö·û´®ÖÐÖ¸¶¨µÄ×Ö·û´®
	int	erase_all(std::string& strBuf, const std::string& strDel);

	//std::string to_lower(std::string strBuf);

	int	replace_all(std::string& strBuf, std::string  strSrc, std::string  strDes);
	
	bool is_digits(const std::string& str);

	// ×Ö·û´®×ªÐ¡Ð´
	std::string 	StrToLwr(std::string  strBuf);

	void to_lower(char* str);

	void to_lower(std::string& str);


	// ×Ö·û´®×ª´óÐ´
	std::string 	StrToUpr(std::string  strBuf);

	void to_upper(char* str);

	void to_upper(std::string& str);

	void parseString(const std::string& input, std::string& szSection, std::string& szKey);


	std::string GetCurrentWorkingDirectory();

	std::string IniToJson();

	std::string JsonToIni();
}