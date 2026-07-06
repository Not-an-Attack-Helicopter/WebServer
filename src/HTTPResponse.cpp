#include <sstream>
#include "HTTPResponse.hpp"


//constructors / destructor
HTTPResponse::HTTPResponse() : _status_code(200), _reason_phrase("OK"), _body("") {}

HTTPResponse::~HTTPResponse() {}

HTTPResponse::HTTPResponse(const HTTPResponse& other)
	: _status_code(other._status_code),
	  _reason_phrase(other._reason_phrase),
	  _headers(other._headers),
	  _body(other._body) {}

HTTPResponse& HTTPResponse::operator=(const HTTPResponse& other) {
	if (this != &other) {
		_status_code = other._status_code;
		_reason_phrase = other._reason_phrase;
		_headers = other._headers;
		_body = other._body;
	}
	return *this;
}

std::string HTTPResponse::_defaultReason(int code)
{
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
void HTTPResponse::setStatus(int code, const std::string& reason)
{
	_status_code = code;
	_reason_phrase = reason.empty() ? _defaultReason(code) : reason;
}
void HTTPResponse::setHeader(const std::string& key, const std::string& value)
{
	_headers[key] = value;
}
void HTTPResponse::setBody(const std::string& body, const std::string& content_type)
{
	_body = body;
	setHeader("Content-Type", content_type);
	std::ostringstream oss;
	oss << body.size();
	setHeader("Content-Length", oss.str());
}


// Getters
int                                       HTTPResponse::getStatusCode()   const { return _status_code; }
const std::string&                        HTTPResponse::getReasonPhrase() const { return _reason_phrase; }
const std::string&                        HTTPResponse::getBody()         const { return _body; }
const std::map<std::string, std::string>& HTTPResponse::getHeaders()      const { return _headers; }

// Produce the raw HTTP/1.1 string ready to write to the socket
std::string HTTPResponse::serialize() const
{
	std::ostringstream oss;
	oss << _status_code;
	std::string response = "HTTP/1.1 " + oss.str() + " " + _reason_phrase + "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
		response += it->first + ": " + it->second + "\r\n";
	}
	response += "\r\n" + _body;
	return response;
}

void HTTPResponse::reset()
{
	_status_code = 200;
	_reason_phrase = "OK";
	_headers.clear();
	_body.clear();
}