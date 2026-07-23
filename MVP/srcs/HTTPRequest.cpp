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
#include "../incs/templates.hpp"
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"
#include <sstream>
#include <cstring>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cerrno>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

// DEBUG BEGIN
unsigned long HTTPRequest::global_count = 0;

std::map<std::string, std::string>& HTTPRequest::getHeaders() {
	return _headers;
}
// DEBUG END

/*	@brief Constructor	*/
HTTPRequest::HTTPRequest(void)
// DEBUG BEGIN
	:	HR_object_id(++global_count),
		parses_count(0),
// DEBUG END
		_state(PS_READING_REQUEST_LINE),
		_is_unix_style(true),
		_bytes_read_count(0),
		_header_line_size(0),
		_old_buffer_fill_level(0),
		_request_line_end_pos(0),
		_header_line_end_pos(0),
		_line_end_size(0),
		_blank_line_size(0),
		_headers_start_pos(0),
		_headers_end_pos(0),
		_headers_size(0),
		_body_start_pos(0),
		_content_length(0),
		_request_size(0),
		_body_bytes_received(0) {
	log.debug("HTTPRequest Constructor called");
	_buffer.clear();
	_method.clear();
	_path.clear();
	_query.clear();
	_version.clear();
	_body.clear();
	_headers.clear();
	return;
}

/*	@brief Deconstructor	*/
HTTPRequest::~HTTPRequest(void) {
	log.debug("HTTPRequest Deconstructor called");
	if (!_body_path.empty()) {
	 	std::remove(_body_path.c_str());
	  }
	return;
}

//getters
ParseState HTTPRequest::getState(void) const {
	return _state;
}

bool HTTPRequest::getStyle(void) const {
	return _is_unix_style;
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

const std::string& HTTPRequest::getBody(void) const{
	return _body;
};

const std::string& HTTPRequest::getBodyPath(void) const{
	return _body_path;
};

size_t HTTPRequest::getBytesRead(void) {
	return _bytes_read_count;
}

size_t HTTPRequest::getContentLength(void) const {
	return _content_length;
}

void HTTPRequest::reset(void) {

	_state = PS_READING_REQUEST_LINE;
	_is_unix_style = true;
	_bytes_read_count = 0;
	_header_line_size = 0;
	_old_buffer_fill_level = 0;
	_request_line_end_pos = 0;
	_header_line_end_pos = 0,
	_line_end_size = 0;
	_blank_line_size = 0;
	_headers_start_pos = 0;
	_headers_end_pos = 0;
	_headers_size = 0;
	_body_start_pos = 0;
	_content_length = 0;
	_request_size = 0;
	_body_bytes_received = 0;
	_buffer.clear();
	_method.clear();
	_path.clear();
	_query.clear();
	_version.clear();
	_body.clear();
	if (!_body_path.empty()) {
	 	std::remove(_body_path.c_str());
	 	_body_path.clear();
	 }
	_headers.clear();

	return;

}

// Feed raw bytes; returns the current _state:
ParseState HTTPRequest::parse(const std::string& raw) {

// DEBUG BEGIN
	// log.notice("\n#################################################\n");
	log.debug("request id: " + i2a(HR_object_id) + "\tstate: " + i2a(_state) + "\tparses: " + i2a(parses_count));
	++parses_count;
// DEBUG END

	switch (_state) {

	case PS_READING_REQUEST_LINE:

		return _parseRequestLine(raw);

	case PS_READING_HEADERS:

		return _parseHeaders(raw);

	case PS_READING_BODY:

		return _parseBody(raw);

	case PS_COMPLETE:

		return _state;

	case PS_ERROR:

		reset();
		return _state;

	}

	return _state;

}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Copy Constructor	*/
HTTPRequest::HTTPRequest(const HTTPRequest& other)
	:	HR_object_id(other.HR_object_id),
		parses_count(other.parses_count),
		_state(other._state),
		_is_unix_style(other._is_unix_style),
		_method(other._method),
		_path(other._path),
		_query(other._query),
		_version(other._version),
		_body(other._body),
		_headers(other._headers),
		_bytes_read_count(other._bytes_read_count),
		_header_line_size(other._header_line_size),
		_old_buffer_fill_level(other._old_buffer_fill_level),
		_request_line_end_pos(other._request_line_end_pos),
		_header_line_end_pos(other._header_line_end_pos),
		_line_end_size(other._line_end_size),
		_blank_line_size(other._blank_line_size),
		_headers_start_pos(other._headers_start_pos),
		_headers_end_pos(other._headers_end_pos),
		_headers_size(other._headers_size),
		_body_start_pos(other._body_start_pos),
		_content_length(other._content_length),
		_request_size(other._request_size) {
	log.debug("HTTPRequest Copy Constructor called");
	ssize_t i = -1;
	while (other._buffer[++i])
		_buffer[i] = other._buffer[i];
	return;
}

/*	@brief Copy Assignment Operator	*/
HTTPRequest& HTTPRequest::operator = (const HTTPRequest& other) {
	if (this != &other) {
		HR_object_id = other.HR_object_id;
		parses_count = other.parses_count;
		_state = other._state;
		_is_unix_style = other._is_unix_style;
		_method = other._method;
		_path = other._path;
		_query = other._query;
		_version = other._version;
		_body = other._body;
		_headers = other._headers;
		_bytes_read_count = other._bytes_read_count;
		_bytes_read_count = other._header_line_size;
		_old_buffer_fill_level = other._old_buffer_fill_level;
		_request_line_end_pos = other._request_line_end_pos;
		_header_line_end_pos = other._header_line_end_pos;
		_line_end_size = other._line_end_size;
		_blank_line_size = other._blank_line_size;
		_headers_start_pos = other._headers_start_pos;
		_headers_end_pos = other._headers_end_pos;
		_headers_size = other._headers_size;
		_body_start_pos = other._body_start_pos;
		_content_length = other._content_length;
		_request_size = other._request_size;
	}
	log.debug("HTTPRequest Copy Assignment Operator called");
	ssize_t i = -1;
	while (other._buffer[++i])
		_buffer[i] = other._buffer[i];
	return *this;
}

// parsers
size_t HTTPRequest::_findRequestLineEnd(const std::string& raw) {

	size_t unix_end = raw.find(LF);
	size_t windows_end = raw.find(CRLF);

	// Both not found: incomplete data
	if (unix_end == std::string::npos && windows_end == std::string::npos)
		return std::string::npos;

	// Determine which style to use
	if (unix_end < windows_end) // true if unix style (win = npos) / false if windows style
		return unix_end;

	_is_unix_style = false;
	return windows_end;

}

bool HTTPRequest::_extractTokens(const std::string& line) {

	// Transform to stream
	std::stringstream ss(line);
	if (ss.fail())
		return false;

	// Validate number of tokens
	std::string method, uri, version, extra;
	if (!(ss >> method >> uri >> version)) {
		log.error("too few");
		return false; // too few tokens in request line
	}
	if (ss >> extra) {
		log.error("too many");
		return false; // too many tokens in request line
	}

	// Validate method
	if (method != "GET" && method != "HEAD" && method != "DELETE" && method != "POST") {
		log.error("bad method");
		return false;
	}
	_method = method;

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

	 // Validate HTTP version
	version = trim(version); // strip trailing \r
	if (version != "HTTP/1.1" && version != "HTTP/1.0")
		return false;
	_version = version;

	return true;

}

bool HTTPRequest::_parseHeaderLine(const std::string& line) {

	_header_line_size = 0;

	// Find separator ':'
	size_t colon_pos = line.find(':');
	if (colon_pos == 0) {
		log.error("no key provided");
		return false;
	}
	if (colon_pos == std::string::npos) {
		log.error("colon not found");
		return false;
	}

	std::string key = line.substr(0, colon_pos);
	std::string value = line.substr(colon_pos + 1);

	// Trim whitespaces from key
	key = trim(key);
	if (key.empty()) {
		log.error("empty key");
		return false; // Key is required by header
	}

	// Trim whitespaces from value + Lowercase key for case-insensitive lookup
	value = trim(value);
	for (std::string::iterator it = key.begin(); it != key.end(); ++it)
		*it = std::tolower(static_cast<unsigned char>(*it));
	_headers[key] = value;

	return true;

}

bool HTTPRequest::_extractContentLength(void) {

	std::string value = getHeader("content-length");
	if (value.empty())
		return false; // Non-existent or empty value

	char* endptr;
	_content_length = static_cast<size_t>(strtoul(value.c_str(), &endptr, 10));
	if (std::strcmp(endptr, value.c_str()) == 0 || *endptr != '\0')
		return false; // Malformed Content-Length Header

	return true;

}

ParseState HTTPRequest::_parseRequestLine(const std::string& raw) {
	std::string current_line = _buffer + raw;
	size_t carried_size = _buffer.size();

	// Find where request line ends
	_request_line_end_pos = _findRequestLineEnd(current_line);

	// Calculate headers start position based on line ending style ("\n" or "\r\n")
	_line_end_size = _is_unix_style ? UNIX_LINE_END_SIZE : WINDOWS_LINE_END_SIZE;

	// Detected empty line (before start of request line): not copied into _buffer
	if (_request_line_end_pos == 0) {
		_bytes_read_count = _line_end_size; // set byte count to _line_end_size to drop from data
		return _state;

	// No line feed detected (Data only): wait for more data
	} else if (_request_line_end_pos == std::string::npos) {

		_buffer.append(raw);

		_bytes_read_count = _buffer.size() - _old_buffer_fill_level;

		// log.debug("Bytes read: " + i2a(_bytes_read_count));
		// log.debug("Previous buffer fill level: " + i2a(_old_buffer_fill_level));

		_old_buffer_fill_level = _buffer.size();

		// log.debug("Current buffer fill level: " + i2a(_buffer.size()));
		// log.debug("Current data in buffer (request):\n");
		// log.notice((_buffer));

		return _state;

	// Line feed detected (end of request line): procced with line parsing
	} else {
		size_t consumed_from_raw = _request_line_end_pos + _line_end_size - carried_size;

		_buffer.append(raw, 0, consumed_from_raw);
		// // _buffer.append(LF); DO NOT APPEND LINE FEED!

		_bytes_read_count = consumed_from_raw;

		// log.debug("Bytes read: " + i2a(_bytes_read_count));
		// log.debug("Previous buffer fill level: " + i2a(_old_buffer_fill_level));

		_old_buffer_fill_level = _buffer.size();

		// log.debug("Current buffer fill level: " + i2a(_buffer.size()));
		// log.debug("Current data in buffer (request):\n");
		// log.notice((_buffer));
		// log.debug("Line: " + _buffer);

		// Extract method, path, query, and version
		if (!_extractTokens(_buffer)) {
			log.error("error while parsing request line");
			_state = PS_ERROR;
			return _state;
		}

		_state = PS_READING_HEADERS;
		return _state;
	}

}

ParseState HTTPRequest::_parseHeaders(const std::string& raw) {
	std::string carried_line;
	if (_header_line_size > 0 && _buffer.size() >= _header_line_size) {
		carried_line = _buffer.substr(_buffer.size() - _header_line_size);
	}
	std::string current_line = carried_line + raw;
	size_t carried_size = carried_line.size();

	// Check for line break
	_is_unix_style ?
		_header_line_end_pos = current_line.find(LF) :
		_header_line_end_pos = current_line.find(CRLF);

	// Detected line break at 0 pos (empty line): append and proceed with validity checks
	if (_header_line_end_pos == 0) {
		size_t consumed_from_raw = _line_end_size - carried_size;

		_buffer.append(raw, 0, consumed_from_raw);

		_bytes_read_count = consumed_from_raw;

		// log.debug("Bytes read: " + i2a(_bytes_read_count));
		// log.debug("Previous buffer fill level: " + i2a(_old_buffer_fill_level));

		_old_buffer_fill_level = _buffer.size();

		// log.debug("Current buffer fill level: " + i2a(_buffer.size()));
		// log.debug("Current data in buffer (request):\n");
		// log.notice((_buffer));

		// // return _state; DO NOT RETURN AT THIS LINE!

		// After detecting empty line, check buffer for end of headers
		_is_unix_style ?
			_headers_end_pos = _buffer.find(LF LF) :
			_headers_end_pos = _buffer.find(CRLF CRLF);

		// Detected empty line (before start of request line): invalid request
		// (should never happen)
		if (_headers_end_pos == 0) {

			log.error("unexpected empty line");
			_bytes_read_count = 0;
			_state = PS_ERROR;
			return _state;

		// No empty line detected: expecting more header lines
		// (should not occur: appended line break right before check)
		} else if (_headers_end_pos == std::string::npos) {

			// log.debug("no empty line in buffer \t\t yet");
			return _state;

		// Detected empty line: end of header lines
		} else {

			// Calculate cumulative size of header lines
			_headers_start_pos = _request_line_end_pos + _line_end_size;
			_headers_size = _headers_end_pos - _headers_start_pos + _line_end_size;

			// Check for Host Header (mandatory for GET, POST, DELETE)
			if (!hasHeader("host") || getHeader("host").empty()) {
				log.error("no host header found");
				_state = PS_ERROR;
				return _state;
			}

			// GET, HEAD and DELETE are not designed to carry request bodies
			if (_method == "GET" || _method == "HEAD" || _method == "DELETE") {
				_state = PS_COMPLETE;
				return _state;
			}

			// Extract Content-Length value (mandatory for POST)
			if (!_extractContentLength()) {
				_content_length = 0;
				_state = PS_COMPLETE;
				return _state;
			}

			// A zero-length body is already complete; do not wait for more bytes.
			if (_content_length == 0) {
				_state = PS_COMPLETE;
				return _state;
			}

			_state = PS_READING_BODY;
			return _state;

		}

	// No line break: wait for more data
	} else if (_header_line_end_pos == std::string::npos) {

		_buffer.append(raw);

		_bytes_read_count = raw.size();
		_header_line_size += _bytes_read_count;

		// log.debug("Bytes read: " + i2a(_bytes_read_count));
		// log.debug("Line length: " + i2a(_header_line_size));
		// log.debug("Previous buffer fill level: " + i2a(_old_buffer_fill_level));

		_old_buffer_fill_level = _buffer.size();

		// log.debug("Current buffer fill level: " + i2a(_buffer.size()));
		// log.debug("Current data in buffer (request):\n");
		// log.notice((_buffer));

		return _state;

	// Data detected: proceed with line parsing
	} else {
		size_t consumed_from_raw = _header_line_end_pos + _line_end_size - carried_size;

		_buffer.append(raw, 0, consumed_from_raw);
		// // _buffer.append(LF); DO NOT APPEND LINE FEED!

		_bytes_read_count = consumed_from_raw;
		_header_line_size = carried_size + consumed_from_raw;

		// log.debug("Bytes read: " + i2a(_bytes_read_count));
		// log.debug("Line length: " + i2a(_header_line_size));
		// log.debug("Previous buffer fill level: " + i2a(_old_buffer_fill_level));

		_old_buffer_fill_level = _buffer.size();

		// log.debug("Current buffer fill level: " + i2a(_buffer.size()));
		// log.debug("Current data in buffer (request):\n");
		// log.notice((_buffer));

		// Parse header line
		// std::string line = _buffer.substr(_buffer.size() - _bytes_read_count);
		std::string line = _buffer.substr(_buffer.size() - _header_line_size);
		// log.error(line + i2a(_header_line_size));
		if (!_parseHeaderLine(line)) {
			log.error("error while parsing header line");
			_state = PS_ERROR;
			return _state;
		}

		return _state;

	}

	// // After detecting empty line, check buffer for end of headers
	// _is_unix_style ?
	// 	_headers_end_pos = _buffer.find(LF LF) :
	// 	_headers_end_pos = _buffer.find(CRLF CRLF);
 //
	// // Detected empty line (before start of request line): invalid request
	// // (should never happen)
	// if (_headers_end_pos == 0) {
 //
	// 	log.error("unexpected empty line");
	// 	_bytes_read_count = 0;
	// 	_state = PS_ERROR;
	// 	return _state;
 //
	// // No empty line detected: expecting more header lines
	// // (should not occur: return after line parsing)
	// } else if (_headers_end_pos == std::string::npos) {
 //
	// 	// log.debug("no empty line in buffer\t\tyet");
	// 	return _state;
 //
	// // Detected empty line: end of header lines
	// } else {
 //
	// 	// Calculate cumulative size of header lines
	// 	_headers_start_pos = _request_line_end_pos + _line_end_size;
	// 	_headers_size = _headers_end_pos - _headers_start_pos + 1;
 //
	// 	// log.debug("_request_line_end_pos: " + i2a(_request_line_end_pos));
	// 	// log.debug("_line_end_size: " + i2a(_line_end_size));
	// 	// log.debug("_headers_start_pos: " + i2a(_headers_start_pos));
	// 	// log.debug("_headers_end_pos: " + i2a(_headers_end_pos));
	// 	// log.debug("_headers_size: " + i2a(_headers_size));
 //
	// 	// Check for Host Header (mandatory for GET, POST, DELETE)
	// 	if (!hasHeader("host") || getHeader("host").empty()) {
	// 		log.error("no host header found");
	// 		_state = PS_ERROR;
	// 		return _state;
	// 	}
 //
	// 	// GET and DELETE are not designed to carry request bodies
	// 	if (_method == "GET" || _method == "DELETE") {
	// 		_state = PS_COMPLETE;
	// 		return _state;
	// 	}
 //
	// 	// Extract Content-Length value (mandatory for POST)
	// 	if (!_extractContentLength()) {
	// 		log.error("no content-length header found");
	// 		_state = PS_ERROR;
	// 		return _state;
	// 	}
 //
	// 	_state = PS_READING_BODY;
	// 	return _state;
 //
	// }

}

ParseState HTTPRequest::_parseBody(const std::string& raw) {

	// Calculate body start position based on line ending style ("\n\n" or "\r\n\r\n")
	_blank_line_size = _is_unix_style ? UNIX_BLANK_LINE_SIZE : WINDOWS_BLANK_LINE_SIZE;
	_body_start_pos = _headers_end_pos + _blank_line_size;
	_request_size = _body_start_pos + _content_length;
// DEBUG BEGIN
	log.debug("_request_line_end_pos: " + i2a(_request_line_end_pos));
	log.debug("_line_end_size: " + i2a(_line_end_size));
	log.debug("_headers_start_pos: " + i2a(_headers_start_pos));
	log.debug("_headers_end_pos: " + i2a(_headers_end_pos));
	log.debug("_headers_size: " + i2a(_headers_size));
	log.debug("_blank_line_size: " + i2a(_blank_line_size));
	log.debug("_body_start_pos: " + i2a(_body_start_pos));
	log.debug("_content_length: " + i2a(_content_length));
	log.debug("_request_size: " + i2a(_request_size));
	log.debug("_buffer.size() + raw.size(): " + i2a(_buffer.size() + raw.size()));
// DEBUG END

	// ---------- Hybrid body path ----------
	// Bodies > 1 MB are streamed to a temp file to avoid large memory allocations.
	if (_content_length > BODY_STREAM_THRESHOLD) {

		// Open temp file on first body chunk
		if (_body_path.empty()) {
			static unsigned long counter = 0;
			std::ostringstream oss;
			oss << "/tmp/webserv-TMP-" << ++counter;
			_body_path = oss.str();
		}

		// Write only the bytes needed to finish the body
		size_t remaining = _content_length - _body_bytes_received;
		_bytes_read_count = raw.size() < remaining ? raw.size() : remaining;

		std::ofstream file(_body_path.c_str(),
			std::ios::binary | std::ios::app);
		if (!file.is_open()) {
			_state = PS_ERROR;
			return _state;
		}
		file.write(raw.data(), _bytes_read_count);
		if (!file.good()) {
			_state = PS_ERROR;
			return _state;
		}
		file.close();

		_body_bytes_received += _bytes_read_count;

		if (_body_bytes_received == _content_length) {
			_body.clear();
			_state = PS_COMPLETE;
		}
		return _state;

	}

	// ---------- Small-body string path (unchanged) ----------

	// Check if complete body received
	_buffer.append(raw);
// DEBUG BEGIN
	// log.debug("_buffer.size(): " + i2a(_buffer.size()));
// DEBUG END

	// Check if complete body received
	if (_buffer.size() < _request_size) {

		_bytes_read_count = _buffer.size() - _old_buffer_fill_level;

		// log.debug("Bytes read: " + i2a(_bytes_read_count));
		// log.debug("Previous buffer fill level: " + i2a(_old_buffer_fill_level));

		_old_buffer_fill_level = _buffer.size();

		// log.debug("Current buffer fill level: " + i2a(_buffer.size()));
		// log.debug("Current data in buffer (request):\n");
		// log.notice((_buffer));

		_state = PS_READING_BODY;
		return _state;  // not enough data for body: wait for more

	} else {

		_body = _buffer.substr(_body_start_pos, _content_length);

		_bytes_read_count = _request_size - _old_buffer_fill_level;

		log.debug("Expected overflow: " + i2a(_buffer.size() - _request_size));

		log.debug("Actual overfloow: " + i2a((_buffer.size() - _old_buffer_fill_level) - _bytes_read_count));

		_state = PS_COMPLETE;
		return _state;

	}

}


// 	// Check if complete body received
// 	if (_buffer.size() + raw.size() < _request_size) {
// 		// _buffer.erase(_old_buffer_fill_level);
// 		_bytes_read_count = 0;
// 		_state = PS_READING_BODY;
// 		return _state;  // not enough data for body: wait for more
//
// 	// Accumulated enough data for body: proceed
// 	} else {
//
// 		_buffer.append(raw);
// // DEBUG BEGIN
// 		// log.debug("_buffer.size(): " + i2a(_buffer.size()));
// // DEBUG END
// 		_bytes_read_count = _buffer.size() - _old_buffer_fill_level;
//
// 		// log.debug("Bytes read: " + i2a(_bytes_read_count));
// 		// log.debug("Previous buffer fill level: " + i2a(_old_buffer_fill_level));
//
// 		_old_buffer_fill_level = _buffer.size();
//
// 		// log.debug("Current buffer fill level: " + i2a(_buffer.size()));
// 		// log.debug("Current data in buffer (request):\n");
// 		// log.notice((_buffer));
//
// 		// Store body (if valid)
// 		_body = _buffer.substr(_body_start_pos, _content_length);
//
// 		// log.debug("Expected overflow: " + i2a(_buffer.size() - _request_size));
//
// 		// size_t tmp = _bytes_read_count;
// 		// _bytes_read_count = _bytes_read_count - (_buffer.size() - _request_size);
// 		_bytes_read_count = _content_length;
//
// 		// log.debug("Actual overflow: " + i2a(tmp - _bytes_read_count));
// 		// _bytes_read_count = _bytes_read_count + _line_end_size - 1;
// 		// _bytes_read_count = _content_length;
//
// 		_state = PS_COMPLETE;
// 		return _state;
//
// 	}
//
// }

// size_t HTTPRequest::_findHeadersEnd(const std::string raw) {
//
// 	if (_is_unix_style)
// 		return raw.find(LF LF);
// 	else
// 		return raw.find(CRLF CRLF);
//
// }

// bool HTTPRequest::_parseHeaders(const std::string raw) {
//
// 	// log.warn("_headers_start_pos: " + i2a(_headers_start_pos));
// 	// log.warn("_headers_size: " + i2a(_headers_size));
// 	// log.warn("buffer size: " + i2a(_buffer.size()));
// 	// log.warn("<substr: " + _buffer);
// 	// std::string substr = _buffer.substr(_headers_start_pos, _headers_size);
// 	// log.warn("substr>: " + substr);
// 	// log.warn("substr size: " + i2a(substr.size()));
// 	// std::istringstream ss(_buffer.substr(_headers_start_pos, _headers_size));
//
// 	std::string substr = raw.substr(_headers_start_pos, _headers_size);
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
// 			break; // Empty line signals end of headers
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
// log.error("state: " + i2a(_state));
// if (_state == PS_COMPLETE)
// 	return _state;

// if (_state == PS_ERROR)
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
// 	_state = PS_READING_REQUEST_LINE;
// 	return _state; // Incomplete; wait for more data
// // 	// __attribute__ ((fallthrough));
// }
// // default:
// // 	Parse request line
// _parsed_bytes_count += request_line_end_pos;
// std::string line = raw.substr(0, request_line_end_pos);
// log.warn("line: " + line);
// if (!_parseRequestLine(line)) {
// 	log.error("error while parsing request line");
// 	_state = PS_ERROR;
// 	return _state;
// }
// // }

// bool HTTPRequest::_parseBody(const std::string& raw, size_t body_start_pos) {
// 	_body = raw.substr(body_start_pos, _content_length);
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
// 	_state = PS_ERROR;
// 	return _state;
// }

// // Get pointer to Content-Length Header
// std::map<std::string, std::string>::iterator it = _headers.find("content-length");
// if (it == _headers.end()) { // should never happen because of previous check
// 	_state = PS_ERROR;
// 	return _state;
// }

// // Extract Content-Length value
// char* endptr;
// size_t content_length = static_cast<size_t>(strtoul(it->second.c_str(), &endptr, 10));
// if (std::strcmp(endptr, it->second.c_str()) == 0 || *endptr != '\0') {
// 	_state = PS_ERROR;
// 	return _state;
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
// 	_content_length = static_cast<size_t>(strtoul(it->second.c_str(), &endptr, 10));
// if (std::strcmp(endptr, it->second.c_str()) == 0 || *endptr != '\0')
// 	return false; // malformed Content-Length
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
