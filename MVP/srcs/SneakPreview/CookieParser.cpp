/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CookieParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 21:06:35 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/28 21:06:36 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

class CookieParser {

public:

	// Parse Cookie header and extract session ID
	static std::string extractSessionId(const std::string& cookieHeader) {

		std::string sessionId;
		size_t pos = cookieHeader.find("SESSIONID=");

		if (pos != std::string::npos) {
		pos += 10; // length of "SESSIONID="
		size_t endPos = cookieHeader.find(";", pos);

		if (endPos == std::string::npos) {
			endPos = cookieHeader.length();
		}

		sessionId = cookieHeader.substr(pos, endPos - pos);
		}

		return sessionId;

	}

	// Parse all cookies into a map
	static void parseCookies(const std::string& cookieHeader,
							std::map<std::string, std::string>& cookies) {

		std::string current = cookieHeader;
		size_t pos = 0;

		while (pos < current.length()) {
		size_t eqPos = current.find("=", pos);
		size_t semiPos = current.find(";", pos);

		if (eqPos == std::string::npos) break;

		if (semiPos == std::string::npos) {
			semiPos = current.length();
		}

		std::string name = current.substr(pos, eqPos - pos);
		std::string value = current.substr(eqPos + 1, semiPos - eqPos - 1);

		// Trim whitespace
		name.erase(0, name.find_first_not_of(" \t"));
		name.erase(name.find_last_not_of(" \t") + 1);

		cookies[name] = value;
		pos = semiPos + 1;
		}

	}

};
