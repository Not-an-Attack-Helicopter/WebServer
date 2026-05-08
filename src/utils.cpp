#include "utils.hpp"

std::string trim(const std::string& str)
{
	size_t first = str.find_first_not_of(" \t\r\n");
	size_t last = str.find_last_not_of(" \t\r\n");
	if (first == std::string::npos || last == std::string::npos)
		return "";
	return str.substr(first, last - first + 1);
}

bool valid_ip(const std::string& ip)
{
	std::istringstream iss(ip);
	std::string token;
	int count = 0;
	while (std::getline(iss, token, '.'))
	{
		if (token.empty() || token.size() > 3)
			return false;
		for (char c : token)
		{
			if (!std::isdigit(c))
				return false;
		}
		int num = std::stoi(token);
		if (num < 0 || num > 255)
			return false;
		count++;
	}
	return count == 4;
}