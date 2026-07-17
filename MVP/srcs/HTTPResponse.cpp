/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/09 01:08:09 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/09 01:08:11 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/HTTPResponse.hpp"
#include "../incs/Logger.hpp"
// #include <cstdint>
#include <sstream>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
HTTPResponse::HTTPResponse(void)
	:	_status_code(200),
		_status_reason("OK"),
		_body("") {
	log.debug("HTTPResponse Constructor called");
	return;
}

/*	@brief Deconstructor	*/
HTTPResponse::~HTTPResponse(void) {
	log.debug("HTTPResponse Deconstructor called");
	return;
}

// Status
unsigned int HTTPResponse::getStatusCode(void) const {
	return _status_code;
}

void HTTPResponse::setStatus(int code) {
	_status_code = code;
	_status_reason = _getDefaultReason(code);
}

void HTTPResponse::setStatus(int code, const std::string& reason) {
	_status_code = code;
	_status_reason = reason;
}
const std::string& HTTPResponse::getStatusReason(void) const {
	return _status_reason;
}

// Body, Content-Type, and Content-Length
const std::string& HTTPResponse::getBody(void) const {
	return _body;
}

void HTTPResponse::setBody(const std::string& body, const std::string& content_type) {

	_body = body;

	setHeader("Content-Type", content_type);

	std::ostringstream oss;
	oss << body.size();
	setHeader("Content-Length", oss.str());

}

// Headers
// const std::map<std::string, std::string>& HTTPResponse::getHeaders(void) const {
// 	return _headers;
// }

void HTTPResponse::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
}


// Produce the raw HTTP/1.1 string ready to write to the socket
std::string HTTPResponse::serialize(void) const {

	std::ostringstream oss;
	oss << _status_code;

	std::string response = "HTTP/1.1 " + oss.str() + " " + _status_reason + "\r\n";

	// for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
	// 	response += it->first + ": " + it->second + "\r\n";
	// }
	if (!_headers.empty()) {
		std::map<std::string, std::string>::const_iterator it = _headers.begin();
		while (it != _headers.end()) {
			response += it->first + ": " + it->second + "\r\n";
			++it;
		}
	}

	if (!_body.empty()) {
		response += "\r\n" + _body;
	}

	return response;

}

void HTTPResponse::reset(void) {
	_status_code = 200;
	_status_reason = "OK";
	_headers.clear();
	_body.clear();
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Copy Constructor	*/
HTTPResponse::HTTPResponse(const HTTPResponse& other)
	:	_status_code(other._status_code),
		_status_reason(other._status_reason),
		_body(other._body) {
	_headers = other._headers;
	return;
}

/*	@brief Copy Assignment Operator	*/
HTTPResponse& HTTPResponse::operator = (const HTTPResponse& other) {
	if (this != &other) {
		_status_code = other._status_code;
		_status_reason = other._status_reason;
		_body = other._body;
		_headers = other._headers;
	}
	return *this;
}

std::string HTTPResponse::_getDefaultReason(int code) {
	switch (code) {
		case 200: return "OK";
		// case 201: return "Created";
		// case 204: return "No Content";
		case 301: return "Moved Permanently";
		// case 302: return "Found";
		// case 304: return "Not Modified";
		case 400: return "Bad Request";
		// case 401: return "Unauthorized";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		// case 413: return "Payload Too Large";
		// case 414: return "URI Too Long";
		case 500: return "Internal Server Error";
		// case 501: return "Not Implemented";
		// case 502: return "Bad Gateway";
		// case 503: return "Service Unavailable";
		case 505: return "HTTP Version Not Supported";
		default:  return "Unknown Status";
	}
}

// Range	Category	Examples
// 1xx	Informational	100 Continue, 101 Switching Protocols
// 2xx	Success	200 OK, 201 Created, 204 No Content
// 3xx	Redirection	301 Moved Permanently, 302 Found, 304 Not Modified
// 4xx	Client Error	400 Bad Request, 401 Unauthorized, 403 Forbidden, 404 Not Found, 429 Too Many Requests
// 5xx	Server Error	500 Internal Server Error, 502 Bad Gateway, 503 Service Unavailable
