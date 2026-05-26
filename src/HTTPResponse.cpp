#include "HTTPResponse.hpp"


//constructors / destructor
HTTPResponse::HTTPResponse() : statusCode(200), contentType("text/html"), body("") {}

HTTPResponse::~HTTPResponse() {}

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
void HTTPResponse::setStatus(int code, const std::string& reason = "")
{
	statusCode = code;
	reasonPhrase = reason.empty() ? _defaultReason(code) : reason;
}
void HTTPResponse::setHeader(const std::string& key, const std::string& value)
{
	headers[key] = value;
}
void HTTPResponse::setBody(const std::string& body, const std::string& content_type = "text/html")
{
	this->body = body;
	setHeader("Content-Type", content_type);
	setHeader("Content-Length", std::to_string(body.size()));
}


// Getters
int                                       HTTPResponse::getStatusCode()   const { return statusCode; }
const std::string&                        HTTPResponse::getReasonPhrase() const { return reasonPhrase; }
const std::string&                        HTTPResponse::getBody()         const { return body; }
const std::map<std::string, std::string>& HTTPResponse::getHeaders()      const { return headers; }

// Produce the raw HTTP/1.1 string ready to write to the socket
std::string HTTPResponse::serialize() const;

void HTTPResponse::reset();