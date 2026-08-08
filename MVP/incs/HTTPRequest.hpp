/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sholz + bstorck <marvin@42.fr>             +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 18:43:14 by sholz             #+#    #+#             */
/*   Updated: 2026/06/30 18:43:18 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <cstddef>
#include <string>

enum LineEnding {
	IS_LF,
	IS_CRLF
};

class HTTPRequest {

public:

	HTTPRequest(void);
	~HTTPRequest(void);

	enum ParseState {
		READING_REQUEST_LINE,
		READING_HEADERS,
		READING_BODY,
		COMPLETE,
		ERROR
	};

	struct ParsingContext {
		ParseState							state;
		LineEnding							line_ending;
		size_t								bytes_read_count;
		size_t								header_line_size;
		size_t								old_buffer_fill_level;
		size_t								request_line_end_pos;
		size_t								header_line_end_pos;
		size_t								line_end_size;
		size_t								blank_line_size;
		size_t								headers_start_pos;
		size_t								headers_end_pos;
		size_t								headers_size;
		size_t								body_start_pos;
		size_t								content_length;
		size_t								request_size;
		std::string							buffer;
		std::streamoff						old_stream_pos;
		std::streamoff						full_body_size;
	};

	struct Body {
		std::stringstream					temp;
		std::ofstream						file;
		size_t								size;
		Sink								sink;
	};

	ParsingContext							parsing;

	Body									body;

	// bool									getStyle(void) const;

	// size_t									getBytesRead(void);
	// ParseState								getState(void) const;
	// using									Config::getMethod;
	const Method&							getMethod(void) const;

	const std::string&						getPath(void) const;
	const std::string&						getQuery(void) const;
	const std::string&						getVersion(void) const;
	const std::string&						getBody(void) const;
	const std::string&						getHeader(const std::string& key) const;

	bool									hasHeader(const std::string& key) const;

	// size_t									getContentLength(void) const;

	// void									setBytesRead(size_t bytes_read_count);
	// void									setState(const ParseState& parse_state);
	void									setMethod(const Method& method);
	// void									setMethod(const std::string& method);
	void									setPath(const std::string&);
	void									setQuery(const std::string&);
	void									setVersion(const std::string&);
	void									setBody(const std::string&);
	void									setHeader(const std::string& key, const std::string& value);

	bool									extractContentLength(void);

	void									reset(void);

	// ParseState								parse(const std::string& raw);

// DEBUG BEGIN
	// static unsigned long					global_count;
	// unsigned long							HR_object_id;
	// unsigned long							parses_count;
	const std::string						getMethodName(void) const;
	std::map<std::string, std::string>&		getHeaders(void);
// DEBUG END

private:

	HTTPRequest(const HTTPRequest& other);
	HTTPRequest& operator = (const HTTPRequest& other);

	Method									_method;

	std::string								_path;
	std::string								_query;
	std::string								_version;

	std::map<std::string, std::string>		_headers;

	std::string								_body;

	// char									_buffer[128];

};

#endif

// #include "templates.tpp"

// #define HT "\t"
// #define CR "\r"
// #define LF "\n"
// #define CRLF "\r\n"
// #define LFLF "\n\n"
// #define CRLFCRLF "\r\n\r\n"
// #define __ " "

// std::map<std::string, std::string>&	getHeaders(void);
// const std::string&		getURI(void) const;
// bool						isComplete(void) const;
// bool						_complete; // serves no purpose!
// std::string				_uri; // never used!
// bool						parse(const std::string& raw);
// bool						_parseRequestLine(const std::string& line);
// bool						_parseHeaderLine(const std::string& line);
// size_t					_findBodyStart(size_t header_end_pos);
// bool						_parseBody(const std::string& raw, size_t header_end_pos);

// size_t									_findRequestLineEnd(const std::string& raw);

// // bool									_matchMethod(const std::string& method);
// bool									_extractTokens(const std::string& line);
// bool									_parseHeaderLine(const std::string& line);
// // bool									_extractContentLength(void);

// ParseState								_parseRequestLine(const std::string& raw);
// ParseState								_parseHeaders(const std::string& raw);
// ParseState								_parseBody(const std::string& raw);

// static const size_t 					LF_SIZE = 1;
// static const size_t 					CRLF_SIZE = 2;
// static const size_t						LF_LF_SIZE = 2;
// static const size_t						CRLF_CRLF_SIZE = 4;

// ParseState								parse_state;

// bool									_is_unix_style;
// LineEnding								_line_ending;

// size_t									_bytes_read_count;
// size_t									_header_line_size;
// size_t									_old_buffer_fill_level;
// size_t									_request_line_end_pos;
// size_t									_header_line_end_pos;
// size_t									_line_end_size;
// size_t									_blank_line_size;
// size_t									_headers_start_pos;
// size_t									_headers_end_pos;
// size_t									_headers_size;
// size_t									_body_start_pos;
// size_t									_content_length;
// size_t									_request_size;

// std::string								_buffer;
