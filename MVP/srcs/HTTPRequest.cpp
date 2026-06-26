/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sholz + bstorck <marvin@42.fr>             +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/25 17:39:21 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/25 17:39:23 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/HTTPRequest.hpp"
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"
#include <sstream>
#include <cstdlib>
#include <cstddef>
#include <cctype>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
HTTPRequest::HTTPRequest(void)
	:	_state(PS_REQUEST_LINE),
		_is_unix_style(true),
		_content_length(0) {
	log.debug("HTTPRequest Constructor called");
	return;
	}

/*	@brief Copy Constructor	*/
HTTPRequest::HTTPRequest(const HTTPRequest& other)
	:	_state(other._state),
		_is_unix_style(other._is_unix_style),
		_headers(other._headers),
		_method(other._method),
		_path(other._path),
		_query(other._query),
		_version(other._version),
		_body(other._body),
		_content_length(other._content_length) {
	log.debug("HTTPRequest Copy Constructor called");
	return;
		}

/*	@brief Copy Assignment Operator	*/
HTTPRequest& HTTPRequest::operator=(const HTTPRequest& other) {
	if (this != &other) {
		_state = other._state;
		_is_unix_style = other._is_unix_style;
		_headers = other._headers;
		_method = other._method;
		_path = other._path;
		_query = other._query;
		_version = other._version;
		_body = other._body;
		_content_length = other._content_length;
	}
	log.debug("HTTPRequest Copy Assignment Operator called");
	return *this;
}

/*	@brief Copy Destructor	*/
HTTPRequest::~HTTPRequest(void) {
	log.debug("HTTPRequest Destructor called");
	return;
}


//getters
ParseState HTTPRequest::getState(void) const {
	return _state;
}

const std::string& HTTPRequest::getMethod(void) const {
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

size_t HTTPRequest::getContentLength(void) const {
	return _content_length;
}

const std::string& HTTPRequest::getBody(void) const{
	return _body;
};


void HTTPRequest::reset(void) {
	_state = PS_REQUEST_LINE;
	_method.clear();
	_path.clear();
	_query.clear();
	_version.clear();
	_headers.clear();
	_content_length = 0;
	_body.clear();
}


// Feed raw bytes; returns the current _state:
ParseState HTTPRequest::parse(const std::string& raw) {

	if (_state == PS_COMPLETE || _state == PS_ERROR)
		return _state;
	reset();

	// Find where headers end + check if all headers received
	size_t header_end_pos = _findHeaderEnd(raw);
	if (header_end_pos == std::string::npos) {
		_state = PS_READING_HEADERS;
		return _state; // Incomplete; wait for more data
	}

	// Parse request line + headers
	if (!_parseHeaders(raw, header_end_pos)) {
		_state = PS_ERROR;
		return _state;
	}

	// Check for Host Header (mandatory for GET, POST, DELETE)
	if (!hasHeader("host") || getHeader("host").empty()) {
		_state = PS_ERROR;
		return _state;
	}

	// GET and DELETE are not designed to carry request bodies
	if (_method == "GET" || _method == "DELETE") {
		_state = PS_COMPLETE;
		return _state;
	}

	// Check for Content-Length Header (mandatory for POST)
	if (!hasHeader("content-length") || getHeader("content-length").empty()) {
		_state = PS_ERROR;
		return _state;
	}

	// Get pointer to Content-Length Header
	std::map<std::string, std::string>::iterator it = _headers.find("content-length");
	if (it == _headers.end()) { // should never happen because of previous check;
		_state = PS_ERROR;
		return _state;
	}

	// Find where body starts + check if complete body received
	size_t body_start_pos = _findBodyStart(it->second, header_end_pos);
	if (raw.size() < body_start_pos + _content_length) {
		_state = PS_READING_BODY;
		return _state;
	}

	// Parse body (if present)
	_body = raw.substr(body_start_pos, _content_length);
	_state = PS_COMPLETE;
	return _state;

}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

// parsers
size_t HTTPRequest::_findHeaderEnd(const std::string& raw) {

	size_t unix_end = raw.find("\n\n");
	size_t windows_end = raw.find("\r\n\r\n");

	// Both not found
	if (unix_end == std::string::npos && windows_end == std::string::npos)
		return std::string::npos;

	// Determine which style to use
	if (unix_end != std::string::npos &&
		(windows_end == std::string::npos || unix_end < windows_end)) {
		_is_unix_style = true;
		return unix_end;
	}

	_is_unix_style = false;
	return windows_end;

}

bool HTTPRequest::_parseHeaders(const std::string& raw, size_t header_end_pos) {

	std::istringstream ss(raw.substr(0, header_end_pos));
	std::string line;

	// Parse request line
	if (!std::getline(ss, line) || !_parseRequestLine(line))
		return false;

	// Parse header lines
	while (std::getline(ss, line)) {
		// Strip trailing \r if present (getline removes \n only)
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length() - 1);

		// Empty line signals end of headers
		if (line.empty())
			break;

		if (!_parseHeaderLine(line))
			return false;
	}

	return true;
}

bool HTTPRequest::_parseRequestLine(const std::string& line) {

	std::stringstream ss(line);
	std::string method, uri, version, extra;

	// Validate number of tokens
	if (!(ss >> method >> uri >> version))
		return false; // too few tokens in request line
	if (ss >> extra)
		return false; // extra tokens in request line

	// Validate method
	if (method != "GET" && method != "DELETE" && method != "POST")
		return false;
	_method = method;

	// Strip trailing \r from version
	if (!version.empty() && version[version.length() - 1] == '\r')
		version.erase(version.length() - 1);

	// Validate HTTP version
	if (version != "HTTP/1.1" && version != "HTTP/1.0")
		return false;
	_version = version;

	// Validate URI
	if (uri.empty() || uri[0] != '/')
		return false;

	// Split URI into path and query
	size_t query_start_pos = uri.find('?');
	if (query_start_pos != std::string::npos) {
		_path = uri.substr(0, query_start_pos);
		_query = uri.substr(query_start_pos + 1);
	} else {
		_path = uri;
		_query.clear();
	}

	return true;

}

bool HTTPRequest::_parseHeaderLine(const std::string& line) {

	size_t colon_pos = line.find(':');
	if (colon_pos == std::string::npos || colon_pos == 0)
		return false;

	std::string key = line.substr(0, colon_pos);
	std::string value = line.substr(colon_pos + 1);

	// Trim whitespaces from key
	if (!key.empty())
		key = trim(key);
	if (key.empty())
		return false;

	// Trim whitespaces from value
	if (!value.empty())
		value = trim(value);

	// Lowercase key for case-insensitive lookup
	for (std::string::iterator it = key.begin(); it != key.end(); ++it)
		*it = std::tolower(static_cast<unsigned char>(*it));

	_headers[key] = value;
	return true;

}

size_t HTTPRequest::_findBodyStart(const std::string& value, size_t header_end_pos) {

	// Extract Content-Length value
	char* endptr;
	_content_length = static_cast<size_t>(strtoul(value.c_str(), &endptr, 10));
	if (endptr == value.c_str() || *endptr != '\0')
		return false; // Malformed Content-Length

	// Calculate body start position based on line ending style ("\n\n" or "\r\n\r\n")
	size_t line_ending_size = _is_unix_style ? UNIX_LINE_ENDING_SIZE : WINDOWS_LINE_ENDING_SIZE;
	return header_end_pos + line_ending_size;
}


// bool HTTPRequest::_parseBody(const std::string& raw, size_t body_start_pos) {
// 	_body = raw.substr(body_start_pos, _content_length);
// 	return true;
// }

// for (size_t i = 0; i < key.length(); ++i)
// 	key[i] = ::tolower(static_cast<unsigned char>(key[i]));

// Trim trailing whitespace and \r from key
// while (!key.empty() && (key[key.length() - 1] == ' ' || key[key.length() - 1] == '\t' ||key[key.length() - 1] == '\r'))
// 	key.erase(key.length() - 1);

// Trim leading whitespace from value
// size_t val_start = value.find_first_not_of(" \t");
// value = (val_start == std::string::npos) ? "" : value.substr(val_start);

// Trim trailing whitespace and \r from value
// while (!value.empty() && (value[value.length() - 1] == ' ' || value[value.length() - 1] == '\t' || value[value.length() - 1] == '\r'))
// 	value.erase(value.length() - 1);

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
// 	if (_state == PS_COMPLETE || _state == PS_ERROR)
// 		return _state;
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
// 			_state = PS_ERROR;
// 			return _state;
// 		}
// 		while (std::getline(ss, line) && line != "\r") {
// 			if (!_parseHeaderLine(line)) {
// 				_state = PS_ERROR;
// 				return _state;
// 			}
// 		}
// 		_state = PS_READING_HEADERS;
// 		return _state;
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
// 		_state = PS_ERROR;
// 		return _state;
// 	}
// 	while (std::getline(ss, line) && line != "\r") {
// 		if (!_parseHeaderLine(line)) {
// 			_state = PS_ERROR;
// 			return _state;
// 		}
// 	}
//
// 	if (!_parseBody(raw, header_end_pos)) {
// 		_state = PS_ERROR;
// 		return _state;
// 	}
// 	_state = PS_COMPLETE;
// 	// _complete = true;
// 	return _state;
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
// 	_method = method;
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

// bool HTTPRequest::_parseBody(const std::string& raw, size_t header_end_pos) {
// 	std::map<std::string, std::string>::iterator it = _headers.find("content-length"); // keys are lowercased
// 	if (it == _headers.end()) {
// 		_body.clear();
// 		return true; // no body (e.g. GET) is valid
// 	}
// 	char* endptr;
// 	_content_length = static_cast<size_t>(strtoul(it->second.c_str(), &endptr, 10));
// 	if (endptr == it->second.c_str() || *endptr != '\0')
// 		return false; // malformed Content-Length
// 	size_t offset = _is_unix_style ? UNIX_STYLE : SLOP_STYLE;
// 	if (raw.size() < header_end_pos + offset + _content_length)
// 		return false; // not enough data received yet
// 	_body = raw.substr(header_end_pos + offset, _content_length);
// 	return true;
// }

// bool HTTPRequest::isComplete(void) const { return _complete; }
// bool HTTPRequest::parse(const std::string& raw) {
// 	if (_state == PS_COMPLETE || _state == PS_ERROR)
// 		return _state == PS_COMPLETE;
//
// 	size_t header_end_pos = raw.find("\r\n\r\n");
// 	if (header_end_pos == std::string::npos) {
// 		std::istringstream ss(raw);
// 		std::string line;
// 		if (!std::getline(ss, line) || !_parseRequestLine(line)) {
// 			_state = PS_ERROR;
// 			return false;
// 		}
// 		while (std::getline(ss, line) && line != "\r") {
// 			if (!_parseHeaderLine(line)) {
// 				_state = PS_ERROR;
// 				return false;
// 			}
// 		}
// 		_state = PS_READING_HEADERS;
// 		return false;
// 	}

// 	// We have a complete header, parse it
// 	std::istringstream ss(raw.substr(0, header_end_pos));
// 	std::string line;
// 	if (!std::getline(ss, line) || !_parseRequestLine(line)) {
// 		_state = PS_ERROR;
// 		return false;
// 	}
// 	while (std::getline(ss, line) && line != "\r") {
// 		if (!_parseHeaderLine(line)) {
// 			_state = PS_ERROR;
// 			return false;
// 		}
// 	}

// 	if (!_parseBody(raw, header_end_pos)) {
// 		_state = PS_ERROR;
// 		return false;
// 	}
// 	_state = PS_COMPLETE;
// 	_complete = true;
// 	return true;
// }
