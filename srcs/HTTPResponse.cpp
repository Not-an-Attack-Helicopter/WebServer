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
#include "../incs/templates.hpp"
#include "../incs/Logger.hpp"
// #include <cstdint>
#include <cstddef>
#include <fstream>
// #include <sstream>

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
	_body_sink = NONE;
	return;
}

/*	@brief Destructor	*/
HTTPResponse::~HTTPResponse(void) {
	log.debug("HTTPResponse Destructor called");
	return;
}

// Status
unsigned int HTTPResponse::getStatusCode(void) const {
	return static_cast<unsigned int>(_status_code);
}

void HTTPResponse::setStatus(StatusCode code) {
	// log.error("status set: " + i2a(code));
	_status_code = code;
	_status_reason = _getDefaultReason(code);
	// log.error(_status_reason);
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

void HTTPResponse::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
	return;
}

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

void HTTPResponse::setBody(const std::string& str,
						   const std::string& content_type,
						   bool headers_only) {

	setHeader("Content-Type", content_type);

	std::ifstream file;
	switch (_body_sink) {

	case HEAP:
		_body_size = str.size();
		break;
	case DISK:
		log.error("set body: data read from file: " + str);
		file.open(str.c_str(), std::ios::binary);
		if (!file.is_open()) {
			log.error("set body: unable to open file");
		}
		file.seekg(0, std::ios::end);
		_body_size = static_cast<std::size_t>(file.tellg());
		file.close();
		break;
	default:
		log.warn("HTTP Response: body type undefined");
		_body_size = 0;
	}

	setHeader("Content-Length", i2a(_body_size));

	if (headers_only) {
		_body_sink = NONE;
		return;
	}

	_body = str;
	return;

}

std::size_t HTTPResponse::getBodySize(void) const {
	return _body_size;
}

void HTTPResponse::reset(void) {
	_status_code = OK;
	_status_reason = _getDefaultReason(OK);
	_headers.clear();
	_body.clear();
	_body_sink = NONE;
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
