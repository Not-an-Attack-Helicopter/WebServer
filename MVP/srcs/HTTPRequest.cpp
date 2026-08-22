/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sholz, bstorck <marvin@42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/25 17:39:21 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/25 17:39:23 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/HTTPRequest.hpp"
// #include "../incs/constexpr.hpp"
// #include "../incs/templates.hpp"
#include "../incs/Logger.hpp"
// #include "../incs/Config.hpp"
// #include "../incs/utils.hpp"
// #include <sstream>
#include <cstring>
// #include <cctype>
#include <cstddef>
#include <cstdlib>

// #include <iostream>

// struct Method {
// 	HTTPRequest::Method method;
// 	const std::string name;
// };
//
// static const Method VALID_METHODS[HTTPRequest::METHOD_COUNT] = {
// 	{HTTPRequest::GET, "GET"},
// 	{HTTPRequest::POST, "POST"},
// 	{HTTPRequest::DELETE, "DELETE"}
// };

// static const std::string VALID_METHODS[HTTPRequest::METHOD_COUNT] = {"GET", "HEAD", "DELETE", "POST", "PUT"};
//
// static bool validateMethodOrder() {
// 	if (VALID_METHODS[HTTPRequest::GET] != "GET") return false;
// 	if (VALID_METHODS[HTTPRequest::POST] != "POST") return false;
// 	if (VALID_METHODS[HTTPRequest::DELETE] != "DELETE") return false;
// 	return true;
// }

// static const std::pair<Method, const char*> VALID_METHODS[METHOD_COUNT] = {
//
// 	std::make_pair(GET, "GET"),
// 	std::make_pair(POST, "POST"),
// 	std::make_pair(DELETE, "DELETE")
//
// };

// static bool matchMethod(const std::string& name, Method& method) {
//
// 	for (size_t i = 0; i < arraySize(VALID_METHODS); ++i) {
// 		// if (VALID_METHODS[i] == name) {
// 			// method = static_cast<HTTPRequest::Method>(i);
// 		// if (VALID_METHODS[i].name == name) {
// 		// 	method = VALID_METHODS[i].method;
// 		if (VALID_METHODS[i].second == name) {
// 			method = VALID_METHODS[i].first;
// 			return true;
// 		}
// 	}
// 	return false;
//
// }

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

// DEBUG BEGIN
// unsigned long HTTPRequest::global_count = 0;

const std::string HTTPRequest::getMethodName(void) const {

	static const std::string valid_methods[METHOD_COUNT] = {"GET", "HEAD", "DELETE", "POST", "PUT"};
	size_t method = static_cast<size_t>(_method);
	if (method < METHOD_COUNT) {
		return valid_methods[method];
	}
	log.warn("HTTP Request: No method found");
	return "N/A";

}

std::map<std::string, std::string>& HTTPRequest::getHeaders() {
	return _headers;
}
// DEBUG END

/*	@brief Constructor	*/
HTTPRequest::HTTPRequest(void) {
// DEBUG BEGIN
	// :	HR_object_id(++global_count),
	// 	parses_count(0) {
// DEBUG END
	// :	parse_state(READING_REQUEST_LINE),
	// 	// _is_unix_style(true),
	// 	line_ending(IS_LF),
	// 	bytes_read_count(0),
	// 	header_line_size(0),
	// 	old_buffer_fill_level(0),
	// 	request_line_end_pos(0),
	// 	header_line_end_pos(0),
	// 	line_end_size(0),
	// 	blank_line_size(0),
	// 	headers_start_pos(0),
	// 	headers_end_pos(0),
	// 	headers_size(0),
	// 	body_start_pos(0),
	// 	content_length(0),
	// 	request_size(0) {
	log.debug("HTTPRequest Constructor called");
	// buffer.clear();
	std::memset(static_cast<void*>(&parsing), 0, sizeof(parsing));
	resolved.domain = NULL;
	resolved.location = NULL;
	resolved.method = METHOD_COUNT;
	body.file_fd = -1;
	headers_only = false;
	created_file = false;
	// debug = randomHexString(4);
	// log.error("I am " + debug);
	// std::memset(static_cast<void*>(&body), 0, sizeof(body));
	// body.temp.clear();
	// body.file.close();
	// body.sink = DISK;
	// body.size = 0;
	_method = METHOD_COUNT;
	_path.clear();
	_query.clear();
	_version.clear();
	// _body.clear();
	_headers.clear();
	return;
}

/*	@brief Deconstructor	*/
HTTPRequest::~HTTPRequest(void) {
	log.debug("HTTPRequest Deconstructor called");
	return;
}

//getters
// HTTPRequest::ParseState HTTPRequest::getState(void) const {
// 	return parse_state;
// }

// bool HTTPRequest::getStyle(void) const {
// 	return _is_unix_style;
// }

const Method& HTTPRequest::getMethod(void) const {
	return _method;
}

const std::string& HTTPRequest::getPath(void) const {
	return _path;
}

const std::string& HTTPRequest::getQuery(void) const {
	return _query;
}

const std::string& HTTPRequest::getVersion(void) const {
	return _version;
}

bool HTTPRequest::hasHeader(const std::string& key) const {
	return _headers.find(key) != _headers.end();
}

const std::string& HTTPRequest::getHeader(const std::string& key) const {
	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	static const std::string empty;
	if (it == _headers.end()) return empty;
	return it->second;
}

const std::stringstream& HTTPRequest::getBody(void) const{
	// return _body;
	return body.temp;
};

// size_t HTTPRequest::getBytesRead(void) {
// 	return bytes_read_count;
// }

// size_t HTTPRequest::getContentLength(void) const {
// 	return content_length;
// }

// void HTTPRequest::setState(const ParseState& parse_state) {
// 	parse_state = parse_state;
// }

void HTTPRequest::setMethod(const Method& method) {
	_method = method;
}

void HTTPRequest::setPath(const std::string& path) {
	_path = path;
}

void HTTPRequest::setQuery(const std::string& query) {
	_query = query;
}

void HTTPRequest::setVersion(const std::string& version) {
	_version = version;
}

void HTTPRequest::setHeader(const std::string& key, const std::string& value) {
	// std::pair header = std::make_pair(key, value);
	// _headers.insert(header);
	_headers[key] = value;
}

// // void HTTPRequest::setBody(const std::string& body) {
// // 	_body = body;
// // }

// void HTTPRequest::setBytesRead(size_t bytes_read_count) {
// 	bytes_read_count = bytes_read_count;
// }

bool HTTPRequest::extractContentLength(void) {

	// log.error("EXTRACTING CONTENT LENGTH");

	if (!hasHeader("content-length")) return false; // non-existent

	// log.error("hasContentLengthHeader");
	std::string value = getHeader("content-length");
	if (value.empty())
		return false; // empty value

	// log.error("Content-Length: " + value);
	char* endptr;
	parsing.content_length = static_cast<size_t>(strtoul(value.c_str(), &endptr, 10));
	// log.error("parsing.content_length = " + i2a(parsing.content_length));
	if (std::strcmp(endptr, value.c_str()) == 0 || *endptr != '\0')
		return false; // malformed content-length header

	return true;

}

void HTTPRequest::reset(void) {

	// parse_state = READING_REQUEST_LINE;
	// // _is_unix_style = true;
	// line_ending = IS_LF;
	// bytes_read_count = 0;
	// header_line_size = 0;
	// old_buffer_fill_level = 0;
	// request_line_end_pos = 0;
	// header_line_end_pos = 0,
	// line_end_size = 0;
	// blank_line_size = 0;
	// headers_start_pos = 0;
	// headers_end_pos = 0;
	// headers_size = 0;
	// body_start_pos = 0;
	// content_length = 0;
	// request_size = 0;
	// buffer.clear();
	std::memset(static_cast<void*>(&parsing), 0, sizeof(parsing));
	resolved.domain = NULL;
	resolved.location = NULL;
	resolved.method = METHOD_COUNT;
	body.file_fd = -1;
	headers_only = false;
	created_file = false;
	// std::memset(static_cast<void*>(&body), 0, sizeof(body));
	// body.temp.clear();
	// body.file.close();
	// body.sink = DISK;
	// body.size = 0;
	_method = METHOD_COUNT;
	_path.clear();
	_query.clear();
	_version.clear();
	// _body.clear();
	_headers.clear();

	return;

}

// Feed raw bytes; returns the current parse_state:
// HTTPRequest::ParseState HTTPRequest::parse(const std::string& raw) {
//
// // DEBUG BEGIN
// 	// log.notice("\n#################################################\n");
// 	// log.debug("request id: " + i2a(HR_object_id) + "\tstate: " + i2a(parse_state) + "\tparses: " + i2a(parses_count));
// 	// ++parses_count;
// // DEBUG END
//
// 	switch (parse_state) {
//
// 	case READING_REQUEST_LINE:
//
// 		return _parseRequestLine(raw);
//
// 	case READING_HEADERS:
//
// 		return _parseHeaders(raw);
//
// 	case READING_BODY:
//
// 		return _parseBody(raw);
//
// 	case COMPLETE:
//
// 		return parse_state;
//
// 	case ERROR:
//
// 		reset();
// 		return parse_state;
//
// 	default:
//
// 		return parse_state;
//
// 	}
//
// }

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Copy Constructor	*/
HTTPRequest::HTTPRequest(const HTTPRequest& other)
	// :	HR_object_id(other.HR_object_id),
	// 	parses_count(other.parses_count),
	// :	parse_state(other.parse_state),
	// 	line_ending(other.line_ending),
	// 	// _is_unix_style(other._is_unix_style),
	// 	bytes_read_count(other.bytes_read_count),
	// 	header_line_size(other.header_line_size),
	// 	old_buffer_fill_level(other.old_buffer_fill_level),
	// 	request_line_end_pos(other.request_line_end_pos),
	// 	header_line_end_pos(other.header_line_end_pos),
	// 	line_end_size(other.line_end_size),
	// 	blank_line_size(other.blank_line_size),
	// 	headers_start_pos(other.headers_start_pos),
	// 	headers_end_pos(other.headers_end_pos),
	// 	headers_size(other.headers_size),
	// 	body_start_pos(other.body_start_pos),
	// 	content_length(other.content_length),
	// 	request_size(other.request_size),
	:	parsing(other.parsing),
		resolved(other.resolved),
		// body(other.body),
		headers_only(other.headers_only),
		created_file(other.created_file),
		_method(other._method),
		_path(other._path),
		_query(other._query),
		_version(other._version),
		_headers(other._headers) {
		// _body(other._body) {
	log.debug("HTTPRequest Copy Constructor called");
	// ssize_t i = -1;
	// while (other.buffer[++i])
	// 	buffer[i] = other.buffer[i];
	return;
}

/*	@brief Copy Assignment Operator	*/
HTTPRequest& HTTPRequest::operator = (const HTTPRequest& other) {
	if (this != &other) {
		// HR_object_id = other.HR_object_id;
		// parses_count = other.parses_count;
		// parse_state = other.parse_state;
		// line_ending = other.line_ending;
		// // _is_unix_style = other._is_unix_style;
		// bytes_read_count = other.bytes_read_count;
		// header_line_size = other.header_line_size;
		// old_buffer_fill_level = other.old_buffer_fill_level;
		// request_line_end_pos = other.request_line_end_pos;
		// header_line_end_pos = other.header_line_end_pos;
		// line_end_size = other.line_end_size;
		// blank_line_size = other.blank_line_size;
		// headers_start_pos = other.headers_start_pos;
		// headers_end_pos = other.headers_end_pos;
		// headers_size = other.headers_size;
		// body_start_pos = other.body_start_pos;
		// content_length = other.content_length;
		// request_size = other.request_size;
		parsing = other.parsing;
		resolved = other.resolved;
		// body = other.body;
		headers_only = other.headers_only;
		created_file = other.created_file;
		_method = other._method;
		_path = other._path;
		_query = other._query;
		_version = other._version;
		// _body = other._body;
		_headers = other._headers;
	}
	log.debug("HTTPRequest Copy Assignment Operator called");
	// ssize_t i = -1;
	// while (other.buffer[++i])
	// 	buffer[i] = other.buffer[i];
	return *this;
}

// parsers

// 	// Check if complete body received
// 	if (buffer.size() + raw.size() < request_size) {
// 		// buffer.erase(old_buffer_fill_level);
// 		bytes_read_count = 0;
// 		parse_state = READING_BODY;
// 		return parse_state;  // not enough data for body: wait for more
//
// 	// Accumulated enough data for body: proceed
// 	} else {
//
// 		buffer.append(raw);
// // DEBUG BEGIN
// 		// log.debug("buffer.size(): " + i2a(buffer.size()));
// // DEBUG END
// 		bytes_read_count = buffer.size() - old_buffer_fill_level;
//
// 		// log.debug("Bytes read: " + i2a(bytes_read_count));
// 		// log.debug("Previous buffer fill level: " + i2a(old_buffer_fill_level));
//
// 		old_buffer_fill_level = buffer.size();
//
// 		// log.debug("Current buffer fill level: " + i2a(buffer.size()));
// 		// log.debug("Current data in buffer (request):\n");
// 		// log.notice((buffer));
//
// 		// Store body (if valid)
// 		_body = buffer.substr(body_start_pos, content_length);
//
// 		// log.debug("Expected overflow: " + i2a(buffer.size() - request_size));
//
// 		// size_t tmp = bytes_read_count;
// 		// bytes_read_count = bytes_read_count - (buffer.size() - request_size);
// 		bytes_read_count = content_length;
//
// 		// log.debug("Actual overflow: " + i2a(tmp - bytes_read_count));
// 		// bytes_read_count = bytes_read_count + line_end_size - 1;
// 		// bytes_read_count = content_length;
//
// 		parse_state = COMPLETE;
// 		return parse_state;
//
// 	}
//
// }

// size_t HTTPRequest::_findHeadersEnd(const std::string raw) {
//
// 	if (_is_unix_style)
// 		return raw.find(LF ::LF);
// 	else
// 		return raw.find(CRLF ::CRLF);
//
// }

// bool HTTPRequest::_parseHeaders(const std::string raw) {
//
// 	// log.warn("headers_start_pos: " + i2a(headers_start_pos));
// 	// log.warn("headers_size: " + i2a(headers_size));
// 	// log.warn("buffer size: " + i2a(buffer.size()));
// 	// log.warn("<substr: " + buffer);
// 	// std::string substr = buffer.substr(headers_start_pos, headers_size);
// 	// log.warn("substr>: " + substr);
// 	// log.warn("substr size: " + i2a(substr.size()));
// 	// std::istringstream ss(buffer.substr(headers_start_pos, headers_size));
//
// 	std::string substr = raw.substr(headers_start_pos, headers_size);
// 	std::istringstream ss(substr);
// 	if (ss.fail())
// 		return false;
//
// 	std::string line;
// 	while (std::getline(ss, line)) {
//
// 		// Strip trailing \r
// 		line = trim(line);
// 		if (line.empty())
// 			break; // empty line signals end of headers
//
// 		if (!_parseHeaderLine(line))
// 			return false;
//
// 	}
//
// 	return true;
//
// }

// ++parses_count;
// log.error("request id: " + i2a(HR_object_id) + " number of parses: " + i2a(parses_count));
// log.error("state: " + i2a(parse_state));
// if (parse_state == COMPLETE)
// 	return parse_state;

// if (parse_state == ERROR)
// 	reset();

// reset();

// // Find where request line ends
// size_t request_line_end_pos = _findRequestLineEnd(raw);

// // size_t i = 0;
// // std::string buff = raw;
// // size_t request_line_end_pos = _findRequestLineEnd(buff);;
// // log.warn("pos: " + i2a(request_line_end_pos));
// // while (!request_line_end_pos) {
// // 	++i;
// // 	log.warn("i: " + i2a(i));
// // 	buff = buff.substr(1);
// // 	log.warn("buff: " + buff);
// // 	request_line_end_pos = _findRequestLineEnd(buff);
// // }
// // log.warn("pos: " + i2a(request_line_end_pos));
// // log.warn("buff: " + buff);

// // _parsed_bytes_count = 1;

// // std::string buff = raw;
// // switch (request_line_end_pos) {
// // case 0:
// if (request_line_end_pos == 0) {
// 	std::string buff = raw;
// 	while (!request_line_end_pos) {
// 		log.error("empty line");
// 		buff = buff.substr(1);
// 		++_parsed_bytes_count;
// 		request_line_end_pos = _findRequestLineEnd(buff);
// 	}
// }
// 	// __attribute__ ((fallthrough));
// // case std::string::npos:
// if (request_line_end_pos == std::string::npos) {
// 	parse_state = READING_REQUEST_LINE;
// 	return parse_state; // Incomplete; wait for more data
// // 	// __attribute__ ((fallthrough));
// }
// // default:
// // Parse request line
// _parsed_bytes_count += request_line_end_pos;
// std::string line = raw.substr(0, request_line_end_pos);
// log.warn("line: " + line);
// if (!_parseRequestLine(line)) {
// 	log.error("error while parsing request line");
// 	parse_state = ERROR;
// 	return parse_state;
// }
// // }

// bool HTTPRequest::_parseBody(const std::string& raw, size_t body_start_pos) {
// 	_body = raw.substr(body_start_pos, content_length);
// 	return true;
// }

// if (windows_end != std::string::npos &&
// 		(unix_end == std::string::npos || // never true
// 		windows_end < unix_end)) { // true if windows style / false if unix style
// 	_is_unix_style = false;
// 	return windows_end;
// }

// if (unix_end != std::string::npos &&
// 		(windows_end == std::string::npos || // true if unix style / false if windows style
// 		unix_end < windows_end)) { // true if unix style / false if windows style
// 	_is_unix_style = true;
// 	return unix_end;
// }

// if (windows_end < unix_end) // true if windows style (by definition)/ false if unix style
// 	return windows_end;

// for (size_t i = 0; i < key.length(); ++i)
// 	key[i] = ::tolower(static_cast<unsigned char>(key[i]));

// if (!line.empty() && line[line.length() - 1] == '\r')
// 	line.erase(line.length() - 1);

// if (!version.empty() && version[version.length() - 1] == '\r')
// 	version.erase(version.length() - 1);

// Trim trailing whitespace and \r from key
// while (!key.empty() && (key[key.length() - 1] == ' ' || key[key.length() - 1] == '\t' ||key[key.length() - 1] == '\r'))
// 	key.erase(key.length() - 1);

// Trim leading whitespace from value
// size_t val_start = value.find_first_not_of(" \t");
// value = (val_start == std::string::npos) ? "" : value.substr(val_start);

// Trim trailing whitespace and \r from value
// while (!value.empty() && (value[value.length() - 1] == ' ' || value[value.length() - 1] == '\t' || value[value.length() - 1] == '\r'))
// 	value.erase(value.length() - 1);

// Check for Content-Length Header (mandatory for POST)
// if (!hasHeader("content-length") || getHeader("content-length").empty()) {
// 	parse_state = ERROR;
// 	return parse_state;
// }

// // Get pointer to Content-Length Header
// std::map<std::string, std::string>::iterator it = _headers.find("content-length");
// if (it == _headers.end()) { // should never happen because of previous check
// 	parse_state = ERROR;
// 	return parse_state;
// }

// // Extract Content-Length value
// char* endptr;
// size_t content_length = static_cast<size_t>(strtoul(it->second.c_str(), &endptr, 10));
// if (std::strcmp(endptr, it->second.c_str()) == 0 || *endptr != '\0') {
// 	parse_state = ERROR;
// 	return parse_state;
// }

// // Determine which style to use (prefer Windows if both exist, but Windows comes first)
// if (windows_end != std::string::npos &&
// 	(unix_end == std::string::npos || windows_end < unix_end)) {
// 	_is_unix_style = false;
// 	return windows_end;
// }

// _is_unix_style = true;
// return unix_end;

// const std::map<std::string, std::string>& HTTPRequest::getHeaders(void) const {
// 	return _headers;
// }

// int HTTPRequest::parse(const std::string& raw) {
// 	if (parse_state == COMPLETE || parse_state == ERROR)
// 		return parse_state;
//
// 	size_t header_end_pos = std::string::npos;
// 	size_t unix_line_end_pos = raw.find("\n\n"); // Unix style
// 	size_t slop_line_end_pos = raw.find("\r\n\r\n"); // Windows style
//
// 	if (unix_line_end_pos == std::string::npos && slop_line_end_pos == std::string::npos) {
// 		// header_end_pos = std::string::npos;
// 		std::istringstream ss(raw);
// 		std::string line;
// 		if (!std::getline(ss, line) || !_parseRequestLine(line)) {
// 			parse_state = ERROR;
// 			return parse_state;
// 		}
// 		while (std::getline(ss, line) && line != "\r") {
// 			if (!_parseHeaderLine(line)) {
// 				parse_state = ERROR;
// 				return parse_state;
// 			}
// 		}
// 		parse_state = READING_HEADERS;
// 		return parse_state;
// 	} else if (unix_line_end_pos == std::string::npos) {
// 		_is_unix_style = false;
// 		header_end_pos = slop_line_end_pos;
// 	} else {
// 		header_end_pos = unix_line_end_pos;
// 	}
//
// 	// We have a complete header, parse it
// 	std::istringstream ss(raw.substr(0, header_end_pos));
// 	std::string line;
// 	if (!std::getline(ss, line) || !_parseRequestLine(line)) {
// 		parse_state = ERROR;
// 		return parse_state;
// 	}
// 	while (std::getline(ss, line) && line != "\r") {
// 		if (!_parseHeaderLine(line)) {
// 			parse_state = ERROR;
// 			return parse_state;
// 		}
// 	}
//
// 	if (!_parseBody(raw, header_end_pos)) {
// 		parse_state = ERROR;
// 		return parse_state;
// 	}
// 	parse_state = COMPLETE;
// 	// _complete = true;
// 	return parse_state;
// }

// bool HTTPRequest::_parseRequestLine(const std::string& line) {
// 	std::stringstream ss(line);
// 	std::string method, uri, version, extra;
// 	if (!(ss >> method >> uri >> version))
// 		return false;
// 	if (ss >> extra)
// 		return false; // extra tokens on request line
// 	if (!version.empty() && version[version.size() - 1] == '\r')
// 			version.resize(version.size() - 1);
// 	if (version != "HTTP/1.1" && version != "HTTP/1.0")
// 		return false;
// 	if (uri.empty() || uri[0] != '/')
// 		return false;
// // 	_method = method;
// if (method != "GET" && method != "DELETE" && method != "POST") {
// 	log.error("bad method");
// 	return false;
// }
// if (method == "GET") {
// 	_method = GET;
// } else if (method == "POST") {
// 	_method = POST;
// } else {
// 	_method = DELETE;
// }
// switch (method.length()) {
// case 3:
// 	_method = GET;
// 	break;
// case 4:
// 	_method = POST;
// 	break;
// case 6:
// 	_method = DELETE;
// 	break;
// }
// const std::string valid_methods[5] = {"GET", "HEAD", "DELETE", "POST", "PUT"};
// bool found = false;
// for (size_t i = 0; i < arraySize(valid_methods); ++i) {
// 	if (valid_methods[i] == method) {
// 		_method = static_cast<Method>(i);
// 		found = true;
// 		break;
// 	}
// }
// // if (i >= arraySize(valid_methods)) {
// if (!found) {
// 	log.error("bad method");
// 	return false;
// }
// 	_uri = uri;
// 	_version = version;
// 	size_t query_pos = uri.find('?');
// 	if (query_pos != std::string::npos) {
// 		_path = uri.substr(0, query_pos);
// 		_query = uri.substr(query_pos + 1);
// 	} else {
// 		_path = uri;
// 		_query.clear();
// 	}
// 	return true;
// }
//
// bool HTTPRequest::_parseHeaderLine(const std::string& line) {
// 	size_t colon_pos = line.find(':');
// 	if (colon_pos == std::string::npos || colon_pos == 0)
// 		return false;
// 	std::string key = line.substr(0, colon_pos);
// 	std::string value = line.substr(colon_pos + 1);
// 	// trim trailing whitespace/\r from key
// 	while (!key.empty() && (key[key.size() - 1] == ' ' || key[key.size() - 1] == '\t' || key[key.size() - 1] == '\r'))
// 			key.resize(key.size() - 1);
// 	if (key.empty())
// 		return false;
// 	// trim leading whitespace from value
// 	size_t val_start = value.find_first_not_of(" \t");
// 	value = (val_start == std::string::npos) ? "" : value.substr(val_start);
// 	// trim trailing whitespace/\r from value
// 	while (!value.empty() && (value[value.size() - 1] == '\r' || value[value.size() - 1] == ' ' || value[value.size() - 1] == '\t'))
// 			value.resize(value.size() - 1);
// 	// lowercase key for case-insensitive lookup
// 	std::transform(key.begin(), key.end(), key.begin(), ::tolower);
// 	_headers[key] = value;
// 	return true;
// }

// size_t HTTPRequest::_findBodyStart(size_t header_end_pos) {
//
// 	// Calculate body start position based on line ending style ("\n\n" or "\r\n\r\n")
// 	size_t line_ending_size = _is_unix_style ? UNIX_LINE_ENDING_SIZE : WINDOWS_LINE_ENDING_SIZE;
//
// 	return header_end_pos + line_ending_size;
// }

// bool HTTPRequest::_parseBody(const std::string& raw, size_t header_end_pos) {
// 	std::map<std::string, std::string>::iterator it = _headers.find("content-length"); // keys are lowercased
// 	if (it == _headers.end()) {
// 		_body.clear();
// 		return true; // no body (e.g. GET) is valid
// 	}
// 	char* endptr;
// 	content_length = static_cast<size_t>(strtoul(it->second.c_str(), &endptr, 10));
// if (std::strcmp(endptr, it->second.c_str()) == 0 || *endptr != '\0')
// 	return false; // malformed Content-Length
// 	size_t offset = _is_unix_style ? UNIX_STYLE : SLOP_STYLE;
// 	if (raw.size() < header_end_pos + offset + content_length)
// 		return false; // not enough data received yet
// 	_body = raw.substr(header_end_pos + offset, content_length);
// 	return true;
// }

// bool HTTPRequest::isComplete(void) const { return _complete; }
// bool HTTPRequest::parse(const std::string& raw) {
// 	if (parse_state == COMPLETE || parse_state == ERROR)
// 		return parse_state == COMPLETE;
//
// 	size_t header_end_pos = raw.find("\r\n\r\n");
// 	if (header_end_pos == std::string::npos) {
// 		std::istringstream ss(raw);
// 		std::string line;
// 		if (!std::getline(ss, line) || !_parseRequestLine(line)) {
// 			parse_state = ERROR;
// 			return false;
// 		}
// 		while (std::getline(ss, line) && line != "\r") {
// 			if (!_parseHeaderLine(line)) {
// 				parse_state = ERROR;
// 				return false;
// 			}
// 		}
// 		parse_state = READING_HEADERS;
// 		return false;
// 	}

// 	// We have a complete header, parse it
// 	std::istringstream ss(raw.substr(0, header_end_pos));
// 	std::string line;
// 	if (!std::getline(ss, line) || !_parseRequestLine(line)) {
// 		parse_state = ERROR;
// 		return false;
// 	}
// 	while (std::getline(ss, line) && line != "\r") {
// 		if (!_parseHeaderLine(line)) {
// 			parse_state = ERROR;
// 			return false;
// 		}
// 	}

// 	if (!_parseBody(raw, header_end_pos)) {
// 		parse_state = ERROR;
// 		return false;
// 	}
// 	parse_state = COMPLETE;
// 	_complete = true;
// 	return true;
// }

// const std::string HTTPRequest::getMethodName(void) const {
// // return VALID_METHODS[static_cast<size_t>(_method)];
// // return VALID_METHODS[static_cast<size_t>(_method)].name;
// for (size_t i = 0; i < static_cast<size_t>(METHOD_COUNT); ++i) {
// 	if (VALID_METHODS[i].first == _method) {
// 		return VALID_METHODS[i].second;
// 	}
// }
// throw std::runtime_error("Method not defined");
// }
