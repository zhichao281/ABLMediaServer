#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <filesystem>
#include "openssl/md5.h"


std::map<std::string, std::string> ReadAuth(std::string  line)
{
	std::map<std::string, std::string> name_to_key;

	const size_t sep = line.find('=');
	if (sep == std::string::npos)
		return name_to_key;
	char buf[32];
	size_t len = rtc::hex_decode(rtc::ArrayView<char>(buf),
		absl::string_view(line).substr(sep + 1));
	if (len > 0) {
		name_to_key.emplace(line.substr(0, sep), std::string(buf, len));
	}
	//std::string str = std::string(buf, len);
	//std::cout << "std::string(buf, len) = " << std::string(buf, len).c_str() << std::endl;
	return name_to_key;
}

std::string CalculateMD5(const std::string& input) {
	MD5_CTX md5_context;
	MD5_Init(&md5_context);
	MD5_Update(&md5_context, input.c_str(), input.length());

	unsigned char md5_digest[MD5_DIGEST_LENGTH];
	MD5_Final(md5_digest, &md5_context);

	std::stringstream ss;
	for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
		ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md5_digest[i]);
	}

	return ss.str();
}


std::string  setAuth(std::string username, std::string realm)
{
	std::string md5_result = "";
	// 如果找到了user和realm的值，则进行拼接
	if (!username.empty() && !realm.empty())
	{
		std::string delimiter = ":";
		std::regex regex(delimiter);
		std::sregex_token_iterator it(username.begin(), username.end(), regex, -1);
		std::sregex_token_iterator end;
		std::vector<std::string> userParts(it, end);
		if (userParts.size() == 2) {
			std::string result = userParts[0] + ":" + realm + ":" + userParts[1];
		//	std::cout << "Result: " << result << std::endl;
			md5_result = CalculateMD5(result);
			md5_result = "admin=" + md5_result;
		}
		else {
			std::cerr << "Error: Invalid user format." << std::endl;
		}
	}
	else {
		std::cerr << "Error: User or realm not provided." << std::endl;
	}
	return md5_result;

}