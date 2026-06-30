#pragma once
#include <string>
#include <map>

class HTTPResponse {

public:

	HTTPResponse(void);
	~HTTPResponse(void);

	// Setters
	void											setStatus(int code, const std::string& reason = "");
	void											setHeader(const std::string& key, const std::string& value);
	void											setBody(const std::string& body, const std::string& content_type = "text/html");

	// Getters
	unsigned int									getStatusCode(void) const;

	const std::string&								getReasonPhrase(void) const;
	const std::string&								getBody(void) const;

	const std::map<std::string, std::string>&		getHeaders(void) const;

	// Produce the raw HTTP/1.1 string ready to write to the socket
	std::string										serialize(void) const;

	void reset(void);

private:

	HTTPResponse(const HTTPResponse& other);
	HTTPResponse& operator = (const HTTPResponse& other);

	static std::string							_defaultReason(int code);

	int											_status_code;

	std::map<std::string, std::string>			_headers;
	std::string									_body;
	std::string									_reason_phrase;

};
