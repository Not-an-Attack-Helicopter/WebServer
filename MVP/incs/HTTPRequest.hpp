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
// #include "Buffer.hpp"
#include "HTTPResponse.hpp"
#include "HTTPParameters.hpp"
// #include "MultipartBody.hpp"
// #include <fstream>
#include <sstream>
#include <cstddef>
#include <string>

class HTTPRequest {

public:

	HTTPRequest(void);
	~HTTPRequest(void);

	enum LineEnding {
		LF,
		CRLF
	};

	enum State {
		READING_REQUEST_LINE,
		READING_HEADERS,
		READING_BODY,
		DISPATCHING,
		// FINALIZING,
		COMPLETE,
		ERROR
	};

	enum CState {
		READING_SIZE,
		READING_DATA,
		LINE_BREAK,
		READING_TRAILERS,
		END_OF_CHUNKS,
		// ERROR
	};

	enum MPState {
		READING_PREAMBLE,
		READING_PART_HEADERS,
		READING_PART_BODY,
		READING_BOUNDARY,
		END_OF_PART,
		FAILURE
	};

	struct ParsingContext {

		State								state;
		StatusCode							error_cause;
		LineEnding							line_ending;
		size_t								line_end_pos;
		size_t								line_end_size;
		size_t								blank_line_size;
		size_t								bytes_read_count;
		size_t								bytes_written_count;
		// size_t								old_buffer_fill_level;
		// size_t								request_line_end_pos;
		// size_t								headers_start_pos;
		// size_t								header_line_start_pos;
		// size_t								header_line_end_pos;
		// size_t								header_line_size;
		// size_t								headers_end_pos;
		size_t								headers_size;
		size_t								body_size;
		// size_t								body_start_pos;
		// size_t								content_length;
		// size_t								request_size;
		// std::string							buffer;
		CState								chunk_state;
		size_t								chunk_size;
		size_t								chunk_read;
		MPState								multipart_state;
		// std::streamoff						old_stream_pos;
		// std::streamoff						full_body_size;

		ParsingContext(void)
			:	state(READING_REQUEST_LINE),
				error_cause(NO_STATUS),
				line_ending(LF),
				line_end_pos(0),
				line_end_size(0),
				blank_line_size(0),
				bytes_read_count(0),
				bytes_written_count(0),
				headers_size(0),
				body_size(0),
				chunk_state(READING_SIZE),
				chunk_size(0),
				chunk_read(0),
				multipart_state(READING_PREAMBLE) {}

	};

	// struct MIMEParameter {
	// 	std::string										name;
	// 	std::string										value;
	// };

	struct BodyPart {

		std::string										disposition;
		std::string										name;
		std::string										filename;
		// std::string										extension;

		std::string										type;
		std::vector<HTTPParameters::MIMEParameter>		t_parameters;

		std::string										temp;
		std::string										path;
		int												file;
		// size_t											size;
		Sink											sink;

		const std::string*								getHeader(const std::string& key) const;
		// const std::map<std::string, std::string>& 		getHeaders(void) const;
		void											setHeader(const std::string& key,
																  const std::string& value);

		BodyPart(void)
			:	disposition(""),
				name(""),
				filename(""),
				// extension(""),
				type(""),
				temp(""),
				path(""),
				file(0),
				sink(NONE) {
			t_parameters.clear();
			_headers.clear();
		}

	private:
		std::map<std::string, std::string>				_headers;

	};

	// struct BodyChunk {
	// 	size_t size = 0;
	// 	size_t read = 0;
	// };

	struct RequestBody {

		size_t											size;

		std::string										type;
		std::string										boundary;
		std::vector<HTTPParameters::MIMEParameter>		t_parameters;

		std::string										disposition;
		std::string										name;
		std::string										filename;
		// std::string										extension;
		std::string										filenameStar;
		std::vector<HTTPParameters::MIMEParameter>		d_parameters;

		std::string										temp;
		std::string										path;
		int												file;
		// size_t											size;
		Sink											sink;

		std::vector<BodyPart>							parts;

		const std::string*								getHeader(const std::string& key) const;
		void											setHeader(const std::string& key,
																  const std::string& value);

		RequestBody(void)
			:	size(0),
				type(""),
				boundary(""),
				disposition(""),
				name(""),
				filename(""),
				// extension(""),
				filenameStar(""),
				temp(""),
				path(""),
				file(0),
				sink(NONE) {
			t_parameters.clear();
			d_parameters.clear();
			parts.clear();
			_trailers.clear();
		}

	private:
		std::map<std::string, std::string>				_trailers;

	};

	// Buffer												decoded;

	struct ResolvedRoute {

		Method											method;
		const Config::Domain*							domain;
		const Config::Location*							location;
		std::string										path;

		ResolvedRoute(void)
			:	method(METHOD_COUNT),
				domain(NULL),
				location(NULL),
				path("") {}

	};

	static const size_t						MAX_REQUEST_LINE_LENGTH = 4*1024;
	static const size_t						MAX_HEADER_LINE_LENGTH = 8*1024;
	static const size_t						MAX_TOTAL_HEADERS_SIZE = 32*1024;

	ParsingContext							parsing;

	ResolvedRoute							resolved;

	RequestBody								body;

	bool									headers_only; // HEAD method
	bool									is_multipart;
	bool									body_chunked;
	bool									created_file;
	// std::string								debug;

	// bool									getStyle(void) const;

	// size_t									getBytesRead(void);
	// ParseState								getState(void) const;
	// using									Config::getMethod;
	const Method&							getMethod(void) const;

	const std::string&						getPath(void) const;
	const std::string&						getQuery(void) const;
	const std::string&						getVersion(void) const;
	const std::string*						getHeader(const std::string& key) const;
	// const std::map<std::string,
	// 			   std::string>&			getHeaders(void) const;

	const std::stringstream&				getBody(void) const;

	// bool									hasHeader(const std::string& key) const;

	// size_t									getContentLength(void) const;

	// void									setBytesRead(size_t bytes_read_count);
	// void									setState(const ParseState& parse_state);
	void									setMethod(const Method& method);
	// void									setMethod(const std::string& method);
	void									setPath(const std::string&);
	void									setQuery(const std::string&);
	void									setVersion(const std::string&);
	// void									setBody(const std::string&);
	void									setHeader(const std::string& key, const std::string& value);

	void									extractBoundary(std::string value);

	// bool									extractContentType(bool from_part);
	bool									extractContentLength(void);
	// bool									extractContentDisposition(void);

	void									reset(void);

	// ParseState								parse(const std::string& raw);

// DEBUG BEGIN
	// static unsigned long					global_count;
	// unsigned long							HR_object_id;
	// unsigned long							parses_count;
	// const std::string						getMethodName(void) const;
// DEBUG END

private:

	HTTPRequest(const HTTPRequest& other);
	HTTPRequest& operator = (const HTTPRequest& other);

	Method									_method;

	std::string								_path;
	std::string								_query;
	std::string								_version;

	std::map<std::string,
			 std::string>					_headers;

	// std::string								_body;

	// Body									_body;

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
