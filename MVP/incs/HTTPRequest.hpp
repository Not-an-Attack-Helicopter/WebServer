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

#include <cstddef>
#include <string>
#include <map>
#include <fstream>

#define HT "\t"
#define LF "\n"
#define CR "\r"
#define LFLF "\n\n"
#define CRLF "\r\n"
#define SPACE " "

enum ParseState {
	PS_READING_REQUEST_LINE,
	PS_READING_HEADERS,
	PS_READING_BODY,
	PS_COMPLETE,
	PS_ERROR
};

class HTTPRequest {

	public:
		HTTPRequest(void);
		~HTTPRequest(void);

		// Getters
		ParseState				getState(void) const;

		bool					getStyle(void) const;

		const std::string&		getMethod(void) const;
		const std::string&		getPath(void) const;
		const std::string&		getQuery(void) const;
		const std::string&		getVersion(void) const;
		bool					hasHeader(const std::string& key) const;
		const std::string&		getHeader(const std::string& key) const;
		const std::string&		getBody(void) const;
		const std::string&		getBodyPath(void) const;

		size_t					getBytesRead(void);
		size_t					getContentLength(void) const;

		void					reset(void);

		ParseState				parse(const std::string& raw);

// DEBUG BEGIN
		static unsigned long	global_count;
		unsigned long			HR_object_id;
		unsigned long			parses_count;
		std::map<std::string, std::string>&	getHeaders(void);
// DEBUG END

	private:

		HTTPRequest(const HTTPRequest& other);
		HTTPRequest& operator = (const HTTPRequest& other);

		size_t					_findRequestLineEnd(const std::string& raw);

		bool					_extractTokens(const std::string& line);
		bool					_parseHeaderLine(const std::string& line);
		bool					_extractContentLength(void);

		ParseState				_parseRequestLine(const std::string& raw);
		ParseState				_parseHeaders(const std::string& raw);
		ParseState				_parseBody(const std::string& raw);

		static const size_t 					UNIX_LINE_END_SIZE = 1;
		static const size_t						UNIX_BLANK_LINE_SIZE = 2;
		static const size_t 					WINDOWS_LINE_END_SIZE = 2;
		static const size_t						WINDOWS_BLANK_LINE_SIZE = 4;
		static const size_t						BODY_STREAM_THRESHOLD = 1048576;

		ParseState								_state;

		bool									_is_unix_style;

		std::string								_buffer;
		std::string								_method;
		std::string								_path;
		std::string								_query;
		std::string								_version;
		std::string								_body;
		std::string								_body_path;

		std::map<std::string, std::string>		_headers;

		size_t									_bytes_read_count;
		size_t									_header_line_size;
		size_t									_old_buffer_fill_level;
		size_t									_request_line_end_pos;
		size_t									_header_line_end_pos;
		size_t									_line_end_size;
		size_t									_blank_line_size;
		size_t									_headers_start_pos;
		size_t									_headers_end_pos;
		size_t									_headers_size;
		size_t									_body_start_pos;
		size_t									_content_length;
		size_t									_request_size;
		size_t									_body_bytes_received;

		// char									_buffer[128];

};

// #include "templates.tpp"

#endif


// std::map<std::string, std::string>&	getHeaders(void);
// const std::string&		getURI(void) const;
// bool						isComplete(void) const;
// bool						_complete; // Serves no purpose!
// std::string				_uri; // Never used!
// bool						parse(const std::string& raw);
// bool						_parseRequestLine(const std::string& line);
// bool						_parseHeaderLine(const std::string& line);
// size_t					_findBodyStart(size_t header_end_pos);
// bool						_parseBody(const std::string& raw, size_t header_end_pos);
