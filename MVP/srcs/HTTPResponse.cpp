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
#include <cstddef>
#include <fstream>
#include <sstream>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
HTTPResponse::HTTPResponse(void)
	:	_status_code(OK),
		_status_reason(_getDefaultReason(OK)) {
	log.debug("HTTPResponse Constructor called");
	_headers.clear();
	_body.clear();
	_body_sink = DISK;
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

void HTTPResponse::setStatus(StatusCode code) {
	_status_code = code;
	_status_reason = _getDefaultReason(code);
	return;
}

void HTTPResponse::setStatus(StatusCode code, const std::string& reason) {
	_status_code = code;
	_status_reason = reason;
	return;
}
const std::string& HTTPResponse::getStatusReason(void) const {
	return _status_reason;
}

// Headers
const std::map<std::string, std::string>& HTTPResponse::getHeaders(void) const {
	return _headers;
}

// const std::string& HTTPResponse::getHeader(const std::string& key) const {
// 	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
// 	static const std::string empty;
// 	if (it == _headers.end()) return empty;
// 	return it->second;
// }

void HTTPResponse::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
	return;
}

// bool HTTPResponse::hasHeader(const std::string& key) const {
// 	return _headers.find(key) != _headers.end();
// }

// Body, Content-Type, and Content-Length
Sink HTTPResponse::getBodySink(void) const {
	return _body_sink;
}
void HTTPResponse::setBodySink(Sink body_sink) {
	_body_sink = body_sink;
}

const std::string& HTTPResponse::getBody(void) const {
	return _body;
}

void HTTPResponse::setBody(const std::string& str, const std::string& content_type) {

	_body = str;
	setHeader("Content-Type", content_type);

	std::ostringstream oss;
	if (_body_sink == HEAP) {
		oss << str.size();
	} else if (_body_sink == DISK) {
		std::ifstream file;
		file.open(str.c_str(), std::ios::binary);
		file.seekg(0, std::ios::end);
		_content_length = static_cast<size_t>(file.tellg());
		// oss << file.tellg();
		oss << _content_length;
		// file.seekg(0, std::ios::beg);  // reset to start // necessary?
		file.close();
	} else {
		log.warn("HTTP Response: body type undefined");
		oss << 0;
	}
	setHeader("Content-Length", oss.str());

	return;
}

// void HTTPResponse::setFilePath(const std::string& file_path, const std::string& content_type) {
//
// 	_body = file_path;
//
// 	setHeader("Content-Type", content_type);
//
// 	std::ifstream file;
// 	std::ostringstream oss;
//
// 	file.open(file_path.c_str(), std::ios::binary);
// 	file.seekg(0, std::ios::end);
// 	oss << file.tellg();
// 	// file.seekg(0, std::ios::beg);  // reset to start // necessary?
// 	file.close();
//
// 	setHeader("Content-Length", oss.str());
//
// 	return;
// }

// const std::string& HTTPResponse::getFilePath(void) const {
// 	return _body;
// }

// Produce the raw HTTP/1.1 string ready to write to the socket
// std::string HTTPResponse::serialize(void) const {
//
// 	std::ostringstream oss;
// 	oss << _status_code;
//
// 	std::string response = "HTTP/1.1 " + oss.str() + " " + _status_reason + "\r\n";
//
// 	// for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
// 	// 	response += it->first + ": " + it->second + "\r\n";
// 	// }
// 	if (!_headers.empty()) {
// 		std::map<std::string, std::string>::const_iterator it = _headers.begin();
// 		while (it != _headers.end()) {
// 			response += it->first + ": " + it->second + "\r\n";
// 			++it;
// 		}
// 	}
//
// 	// response += "\r\n";
//
// 	if (!_body.empty()) {
// 		response += "\r\n" + _body + "\r\n";
// 	}
//
// 	log.notice(response);
// 	return response;
//
// }

// void HTTPResponse::setContentLength(void) {
// 	_file.seekg(0, std::ios::end);
// 	content_length = _file.tellg();
// 	_file.seekg(0, std::ios::beg);
// 	return;
// }

size_t HTTPResponse::getContentLength(void) const {
	return _content_length;
}

void HTTPResponse::reset(void) {
	_status_code = OK;
	_status_reason = _getDefaultReason(OK);
	_headers.clear();
	_body.clear();
	_body_sink = DISK;
	return;
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Copy Constructor	*/
HTTPResponse::HTTPResponse(const HTTPResponse& other)
	:	_status_code(other._status_code),
		_status_reason(other._status_reason),
		_headers(other._headers),
		_body_sink(other._body_sink) {
	return;
}

/*	@brief Copy Assignment Operator	*/
HTTPResponse& HTTPResponse::operator = (const HTTPResponse& other) {
	if (this != &other) {
		_status_code = other._status_code;
		_status_reason = other._status_reason;
		_headers = other._headers;
		_body_sink = other._body_sink;
	}
	return *this;
}

// This is bad for perfomance (only do when reverse lookup is needed)
// static const std::pair<HTTPResponse::StatusCode, const char*> STATUS_CODES[] = {
// 	std::make_pair(HTTPResponse::CONTINUE, "CONTINUE")
// };

std::string HTTPResponse::_getDefaultReason(StatusCode code) {
	switch (code) {
	case CONTINUE: return "Continue";
	case SWITCHING_PROTOCOLS: return "Switching Protocols";
	case PROCESSING: return "Processing";
	case EARLY_HINTS: return "Early Hints";
	case OK: return "OK";
	case CREATED: return "Created";
	case ACCEPTED: return "Accepted";
	case NON_AUTHORITATIVE_INFORMATION: return "Non-Authoritative Information";
	case NO_CONTENT: return "No Content";
	case RESET_CONTENT: return "Reset Content";
	case PARTIAL_CONTENT: return "Partial Content";
	case MULTI_STATUS: return "Multi-Status";
	case ALREADY_REPORTED: return "Already Reported";
	case IM_USED: return "IM Used";
	case MULTIPLE_CHOICES: return "Multiple Choices";
	case MOVED_PERMANENTLY: return "Moved Permanently";
	case FOUND: return "Found";
	case SEE_OTHER: return "See Other";
	case NOT_MODIFIED: return "Not Modified";
	case USE_PROXY: return "Use Proxy";
	case TEMPORARY_REDIRECT: return "Temporary Redirect";
	case PERMANENT_REDIRECT: return "Permanent Redirect";
	case BAD_REQUEST: return "Bad Request";
	case UNAUTHORIZED: return "Unauthorized";
	case PAYMENT_REQUIRED: return "Payment Required";
	case FORBIDDEN: return "Forbidden";
	case NOT_FOUND: return "Not Found";
	case METHOD_NOT_ALLOWED: return "Method Not Allowed";
	case NOT_ACCEPTABLE: return "Not Acceptable";
	case PROXY_AUTHENTICATION_REQUIRED: return "Proxy Authentication Required";
	case REQUEST_TIMEOUT: return "Request Timeout";
	case CONFLICT: return "Conflict";
	case GONE: return "Gone";
	case LENGTH_REQUIRED: return "Length Required";
	case PRECONDITION_FAILED: return "Precondition Failed";
	case PAYLOAD_TOO_LARGE: return "Payload Too Large";
	case URI_TOO_LONG: return "URI Too Long";
	case UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
	case RANGE_NOT_SATISFIABLE: return "Range Not Satisfiable";
	case EXPECTATION_FAILED: return "Expectation Failed";
	case IM_A_TEAPOT: return "I'm A Teapot";
	case MISDIRECTED_REQUEST: return "Misdirected Request";
	case UNPROCESSABLE_ENTITY: return "Unprocessable Entity";
	case LOCKED: return "Locked";
	case FAILED_DEPENDENCY: return "Failed Dependency";
	case TOO_EARLY: return "Too Early";
	case UPGRADE_REQUIRED: return "Upgrade Required";
	case PRECONDITION_REQUIRED: return "Precondition Required";
	case TOO_MANY_REQUESTS: return "Too Many Requests";
	case REQUEST_HEADER_FIELDS_TOO_LARGE: return "Request Header Fields Too Large";
	case UNAVAILABLE_FOR_LEGAL_REASONS: return "Unavailable For Legal Reasons";
	case INTERNAL_SERVER_ERROR: return "Internal Server Error";
	case NOT_IMPLEMENTED: return "Not Implemented";
	case BAD_GATEWAY: return "Bad Gateway";
	case SERVICE_UNAVAILABLE: return "Service Unavailable";
	case GATEWAY_TIMEOUT: return "Gateway Timeout";
	case HTTP_VERSION_NOT_SUPPORTED: return "HTTP Version Not Supported";
	case VARIANT_ALSO_NEGOTIATES: return "Variant Also Negotiates";
	case INSUFFICIENT_STORAGE: return "Insufficient Storage";
	case LOOP_DETECTED: return "Loop Detected";
	case NOT_EXTENDED: return "Not Extended";
	case NETWORK_AUTHENTICATION_REQUIRED: return "Network Authentication Required";
	default:  return "Unknown Status";
	}
}

// Range	Category	Examples
// 1xx	Informational	100 Continue, 101 Switching Protocols
// 2xx	Success	200 OK, 201 Created, 204 No Content
// 3xx	Redirection	301 Moved Permanently, 302 Found, 304 Not Modified
// 4xx	Client Error	400 Bad Request, 401 Unauthorized, 403 Forbidden, 404 Not Found, 429 Too Many Requests
// 5xx	Server Error	500 Internal Server Error, 502 Bad Gateway, 503 Service Unavailable
