#pragma once
#include "webserver.hpp"

class HTTPResponse {
private:
    int                                 _status_code;
    std::string                         _reason_phrase;
    std::map<std::string, std::string>  _headers;
    std::string                         _body;

    static std::string _defaultReason(int code);

public:
    HTTPResponse();
	~HTTPResponse();

    // Setters
    void setStatus(int code, const std::string& reason = "");
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body, const std::string& content_type = "text/html");

    // Getters
    int                                       getStatusCode()   const;
    const std::string&                        getReasonPhrase() const;
    const std::string&                        getBody()         const;
    const std::map<std::string, std::string>& getHeaders()      const;

    // Produce the raw HTTP/1.1 string ready to write to the socket
    std::string serialize() const;

    void reset();
};
