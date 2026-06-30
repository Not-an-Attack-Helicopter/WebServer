// #include <cstdint>
#include <sstream>
#include "../incs/HTTPResponse.hpp"

//constructors / destructor
HTTPResponse::HTTPResponse(void) : _status_code(200), _reason_phrase("OK"), _body("") {
	return;
}

HTTPResponse::~HTTPResponse(void) {
	return;
}

std::string HTTPResponse::_defaultReason(int code) {
	switch (code) {
	case 200: return "OK";
	case 400: return "Bad Request";
	case 403: return "Forbidden";
	case 404: return "Not Found";
	case 405: return "Method Not Allowed";
	case 500: return "Internal Server Error";
	case 505: return "HTTP Version Not Supported";
	default:  return "Unknown Status";
	}
}

// Setters
void HTTPResponse::setStatus(int code, const std::string& reason) {
	_status_code = code;
	_reason_phrase = reason.empty() ? _defaultReason(code) : reason;
}

void HTTPResponse::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
}

void HTTPResponse::setBody(const std::string& body, const std::string& content_type) {
	_body = body;
	setHeader("Content-Type", content_type);
	std::ostringstream oss;
	oss << body.size();
	setHeader("Content-Length", oss.str());
}

// Getters
unsigned int HTTPResponse::getStatusCode(void) const {
	return _status_code;
}

const std::string& HTTPResponse::getReasonPhrase(void) const {
	return _reason_phrase;
}

const std::string& HTTPResponse::getBody(void) const {
	return _body;
}

const std::map<std::string, std::string>& HTTPResponse::getHeaders(void) const {
	return _headers;
}

// Produce the raw HTTP/1.1 string ready to write to the socket
std::string HTTPResponse::serialize(void) const {
	std::ostringstream oss;
	oss << _status_code;
	std::string response = "HTTP/1.1 " + oss.str() + " " + _reason_phrase + "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
		response += it->first + ": " + it->second + "\r\n";
	}
	response += "\r\n" + _body;
	return response;
}

void HTTPResponse::reset(void)
{
	_status_code = 200;
	_reason_phrase = "OK";
	_headers.clear();
	_body.clear();
}
