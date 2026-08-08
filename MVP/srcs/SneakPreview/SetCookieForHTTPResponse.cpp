/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SetCookieForHTTPResponse.cpp                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 21:05:50 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/28 21:05:51 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

class HTTPResponse {

public:

	std::string statusLine;
	std::map<std::string, std::string> headers;
	std::string body;

	// Set a cookie in the response
	void setCookie(const std::string& name,
					const std::string& value,
					int maxAgeSeconds = -1,
					bool httpOnly = true,
					const std::string& path = "/") {

		char cookieBuffer[512];

		if (maxAgeSeconds > 0) {
		sprintf(cookieBuffer,
				"%s=%s; Path=%s; Max-Age=%d%s",
				name.c_str(),
				value.c_str(),
				path.c_str(),
				maxAgeSeconds,
				httpOnly ? "; HttpOnly" : "");
		} else {
		sprintf(cookieBuffer,
				"%s=%s; Path=%s%s",
				name.c_str(),
				value.c_str(),
				path.c_str(),
				httpOnly ? "; HttpOnly" : "");

		}

		headers["Set-Cookie"] = std::string(cookieBuffer);
	}

	std::string serialize() {

		std::string response = statusLine + "\r\n";

		for (std::map<std::string, std::string>::iterator it = headers.begin();
			it != headers.end(); ++it) {
		response += it->first + ": " + it->second + "\r\n";
		}

		response += "\r\n" + body;
		return response;

	}

};
