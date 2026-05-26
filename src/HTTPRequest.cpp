#include "HTTPRequest.hpp"

HTTPRequest::HTTPRequest() : _content_length(0), _complete(false), _state(ParseState::REQUEST_LINE) {}
HTTPRequest::HTTPRequest(const HTTPRequest& other) : _method(other._method), _uri(other._uri), _path(other._path), _query(other._query), _version(other._version), _headers(other._headers), _body(other._body), _content_length(other._content_length), _complete(other._complete), _state(other._state) {}
HTTPRequest& HTTPRequest::operator=(const HTTPRequest& other) {
    if (this != &other) {
        _method = other._method;
        _uri = other._uri;
        _path = other._path;
        _query = other._query;
        _version = other._version;
        _headers = other._headers;
        _body = other._body;
        _content_length = other._content_length;
        _complete = other._complete;
        _state = other._state;
    }
    return *this;
}
HTTPRequest::~HTTPRequest() {}


// Private parsing methods

bool    HTTPRequest::_parse_request_line(const std::string& line)
{
	std::stringstream ss(line);
	std::string method, uri, version, extra;
	if (!(ss >> method >> uri >> version))
		return false;
	if (ss >> extra)
		return false; // extra tokens on request line
	if (!version.empty() && version.back() == '\r')
		version.pop_back();
	if (version != "HTTP/1.1" && version != "HTTP/1.0")
		return false;
	if (uri.empty() || uri[0] != '/')
		return false;
	_method = method;
	_uri = uri;
	_version = version;
	size_t query_pos = uri.find('?');
	if (query_pos != std::string::npos) {
		_path = uri.substr(0, query_pos);
		_query = uri.substr(query_pos + 1);
	} else {
		_path = uri;
		_query.clear();
	}
	return true;
}

bool    HTTPRequest::_parse_header_line(const std::string& line)
{
	size_t colon_pos = line.find(':');
	if (colon_pos == std::string::npos || colon_pos == 0)
		return false;
	std::string key = line.substr(0, colon_pos);
	std::string value = line.substr(colon_pos + 1);
	// trim trailing whitespace/\r from key
	while (!key.empty() && (key.back() == ' ' || key.back() == '\t' || key.back() == '\r'))
		key.pop_back();
	if (key.empty())
		return false;
	// trim leading whitespace from value
	size_t val_start = value.find_first_not_of(" \t");
	value = (val_start == std::string::npos) ? "" : value.substr(val_start);
	// trim trailing whitespace/\r from value
	while (!value.empty() && (value.back() == '\r' || value.back() == ' ' || value.back() == '\t'))
		value.pop_back();
	// lowercase key for case-insensitive lookup
	std::transform(key.begin(), key.end(), key.begin(), ::tolower);
	_headers[key] = value;
	return true;
}

bool    HTTPRequest::_parse_body(const std::string& raw, size_t header_end)
{
	auto it = _headers.find("content-length"); // keys are lowercased
	if (it == _headers.end()) {
		_body.clear();
		return true; // no body (e.g. GET) is valid
	}
	try {
		_content_length = std::stoul(it->second);
	} catch (...) {
		return false; // malformed Content-Length
	}
	if (raw.size() < header_end + 4 + _content_length)
		return false; // not enough data received yet
	_body = raw.substr(header_end + 4, _content_length);
	return true;
}


//getters

    const std::string&                        HTTPRequest::getMethod() 	  const { return _method; }
    const std::string&                        HTTPRequest::getUri()           const { return _uri; }
    const std::string&                        HTTPRequest::getPath()          const { return _path; }
    const std::string&                        HTTPRequest::getQuery()         const { return _query; }
    const std::string&                        HTTPRequest::getVersion()       const { return _version; }
    const std::map<std::string, std::string>& HTTPRequest::getHeaders()       const { return _headers; }
    const std::string&                        HTTPRequest::getHeader(const std::string& key) const;
    bool                                      HTTPRequest::hasHeader(const std::string& key) const;
    const std::string&                        HTTPRequest::getBody()          const{ return _body; };
    size_t                                    HTTPRequest::getContentLength() const { return _content_length; }
    bool                                      HTTPRequest::isComplete()       const { return _complete; }
    ParseState                                HTTPRequest::getState()         const { return _state; }


	// Public methods

	bool HTTPRequest::parse(const std::string& raw)
	{
		if (_state == ParseState::COMPLETE || _state == ParseState::ERROR)
			return _state == ParseState::COMPLETE;

		size_t header_end = raw.find("\r\n\r\n");
		if (header_end == std::string::npos) {
			std::istringstream ss(raw);
			std::string line;
			if (!std::getline(ss, line) || !_parse_request_line(line))
				return _state = ParseState::ERROR;
			while (std::getline(ss, line) && line != "\r") {
				if (!_parse_header_line(line))
					return _state = ParseState::ERROR;
			}
			return _state = ParseState::READING_HEADERS; 
		}

		// We have a complete header, parse it
		std::istringstream ss(raw.substr(0, header_end));
		std::string line;
		if (!std::getline(ss, line) || !_parse_request_line(line))
			return _state = ParseState::ERROR;
		while (std::getline(ss, line) && line != "\r") {
			if (!_parse_header_line(line))
				return _state = ParseState::ERROR;
		}

		if (!_parse_body(raw, header_end))
			return _state = ParseState::ERROR;

		return _state = ParseState::COMPLETE;
	}

	void HTTPRequest::reset()
	{
		_method.clear();
		_uri.clear();
		_path.clear();
		_query.clear();
		_version.clear();
		_headers.clear();
		_body.clear();
		_content_length = 0;
		_complete = false;
		_state = ParseState::REQUEST_LINE;
	}