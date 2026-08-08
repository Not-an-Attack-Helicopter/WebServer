/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 20:45:17 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/28 20:45:21 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string>
#include "../incs/HTTPResponse.hpp"

class RequestHandler {

public:

	// Extract session ID from Cookie header
	static std::string extractSessionId(const std::string& requestData) {
	size_t cookiePos = requestData.find("Cookie: ");
	if (cookiePos == std::string::npos) {
	return "";
	}

	cookiePos += 8;
	size_t endPos = requestData.find("\r\n", cookiePos);
	std::string cookieHeader = requestData.substr(cookiePos, endPos - cookiePos);

	return CookieParser::extractSessionId(cookieHeader);
}

	// Extract HTTP method from request
	static std::string extractMethod(const std::string& requestData) {
	size_t spacePos = requestData.find(" ");
	if (spacePos == std::string::npos) {
	return "";
	}
	return requestData.substr(0, spacePos);
}

	// Extract request path from request
	static std::string extractPath(const std::string& requestData) {
	size_t firstSpace = requestData.find(" ");
	size_t secondSpace = requestData.find(" ", firstSpace + 1);

	if (firstSpace == std::string::npos || secondSpace == std::string::npos) {
	return "";
	}

	return requestData.substr(firstSpace + 1, secondSpace - firstSpace - 1);
}

	// Parse HTTP request into components
	static void parseRequest(const std::string& requestData,
	std::string& method,
	std::string& path,
	std::string& sessionId) {
	method = extractMethod(requestData);
	path = extractPath(requestData);
	sessionId = extractSessionId(requestData);
	}

	// Build response for new session
	static void buildNewSessionResponse(const std::string& sessionId,
	HTTPResponse& response) {
	response.statusLine = "HTTP/1.1 200 OK";
	response.headers["Content-Type"] = "text/plain";
	response.headers["Content-Length"] = "21";
	response.setCookie("SESSIONID", sessionId, 3600);
	response.body = "New session created";
	}

	// Build response for existing session
	static void buildExistingSessionResponse(const std::string& userData,
	HTTPResponse& response) {
	response.statusLine = "HTTP/1.1 200 OK";
	response.headers["Content-Type"] = "text/plain";

	std::string body = "Welcome back, " + userData;
	char lengthBuffer[16];
	sprintf(lengthBuffer, "%zu", body.length());
	response.headers["Content-Length"] = lengthBuffer;

	response.body = body;
	}

	// Build response for expired session
	static void buildExpiredSessionResponse(const std::string& sessionId,
	HTTPResponse& response) {
	response.statusLine = "HTTP/1.1 200 OK";
	response.headers["Content-Type"] = "text/plain";
	response.headers["Content-Length"] = "29";
	response.setCookie("SESSIONID", sessionId, 3600);
	response.body = "Session expired, new one created";
	}

	// Build 404 response
	static void buildNotFoundResponse(HTTPResponse& response) {
	response.statusLine = "HTTP/1.1 404 Not Found";
	response.headers["Content-Type"] = "text/plain";
	response.headers["Content-Length"] = "9";
	response.body = "Not Found";
}

};

