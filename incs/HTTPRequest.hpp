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

// #include "Config.hpp"
// #include "Buffer.hpp"
#include "HTTPResponse.hpp"
#include "HTTPParameters.hpp"
#include <netinet/in.h>

class CgiHandler; // full type only needed where we delete it, in HTTPRequest.cpp
// #include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstddef>

class HTTPRequest {

public:

	HTTPRequest(const sockaddr_in* remote_socket, const sockaddr_in* server_socket);
	~HTTPRequest(void);

	enum State {
		READING_REQUEST_LINE,
		READING_HEADERS,
		RESOLVING_ROUTE,
		READING_BODY,
		// CGI_PROCESSING,
		// READING_STDOUT,
		COMPLETE,
		ERROR
	};

	enum CState {
		READING_SIZE,
		READING_DATA,
		READING_TRAILERS,
		LINE_BREAK,
		END_OF_CHUNKS,
	};

	enum MPState {
		PREAMBLE,
		READING_PART_HEADERS,
		READING_PART_BODY,
		BOUNDARY,
		END_OF_PARTS,
	};

	enum LineEnding {
		LF,
		CRLF
	};

	struct ParsingContext {

		State											state;
		CState											chunk_state;
		MPState											multipart_state;
		StatusCode										error_cause;
		LineEnding										line_ending;
		std::size_t										line_end_pos;
		std::size_t										line_end_size;
		std::size_t										blank_line_size;
		std::size_t										bytes_read_count;
		std::size_t										bytes_written_count;
		std::size_t										headers_size;
		std::size_t										body_size;
		std::size_t										chunk_size;
		std::size_t										chunk_read;

		ParsingContext(void)
			:	state(READING_REQUEST_LINE),
				chunk_state(READING_SIZE),
				multipart_state(PREAMBLE),
				error_cause(NO_STATUS),
				line_ending(LF),
				line_end_pos(0),
				line_end_size(0),
				blank_line_size(0),
				bytes_read_count(0),
				bytes_written_count(0),
				headers_size(0),
				body_size(0),
				chunk_size(0),
				chunk_read(0) {}

	};

	struct Cookie {

		std::string										name;
		std::string										value;

		Cookie(void) : name(""), value("") {}

	};

	struct CGIContext {

		sockaddr_in										remote_socket;
		sockaddr_in										server_socket;

		std::string										binary_path;
		std::string										script_name;
		std::string										path_info;

		int												std_out;
		int												std_in;

		CGIContext(void)
			:	binary_path(""),
				script_name(""),
				path_info(""),
				std_out(-1),
				std_in(-1) {
			std::memset(&remote_socket, 0, sizeof(remote_socket));
			std::memset(&server_socket, 0, sizeof(server_socket));
		}

	};

	struct BodyPart {

		std::string										disposition;
		std::string										name;
		std::string										filename;

		std::string										type;
		std::vector<HTTPParameters::MIMEParameter>		t_parameters;

		std::string										temp;
		std::string										path;
		int												file;
		// std::size_t										size;
		Sink											sink;

		const std::string*								getHeader(const std::string& key) const;
		void											setHeader(const std::string& key,
																  const std::string& value);

		BodyPart(void)
			:	disposition(""),
				name(""),
				filename(""),
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

	struct RequestBody {


		std::string										type;
		std::string										boundary;
		std::vector<HTTPParameters::MIMEParameter>		t_parameters;

		std::string										disposition;
		std::string										name;
		std::string										filename;
		std::string										filenameStar;
		std::vector<HTTPParameters::MIMEParameter>		d_parameters;

		int												file;
		std::string										path;
		std::string										temp;
		std::size_t										size;
		Sink											sink;

		std::vector<BodyPart>							parts;

		const std::string*								getHeader(const std::string& key) const;
		void											setHeader(const std::string& key,
																  const std::string& value);

		RequestBody(void)
			:	type(""),
				boundary(""),
				disposition(""),
				name(""),
				filename(""),
				filenameStar(""),
				file(0),
				path(""),
				temp(""),
				size(0),
				sink(NONE) {
			t_parameters.clear();
			d_parameters.clear();
			parts.clear();
			_trailers.clear();
		}

	private:
		std::map<std::string, std::string>				_trailers;

	};

	struct ResolvedRoute {

		Method											method;
		const Config::Domain*							domain;
		const Config::Location*							location;
		std::string										filepath;

		ResolvedRoute(void)
			:	method(METHOD_COUNT),
				domain(NULL),
				location(NULL),
				filepath("") {}

	};

	static const std::size_t							MAX_REQUEST_LINE_LENGTH = 4*1024;
	static const std::size_t							MAX_HEADER_LINE_LENGTH = 8*1024;
	static const std::size_t							MAX_TOTAL_HEADERS_SIZE = 32*1024;
	static const std::size_t							MAX_HEAP_STORED_BODY_SIZE = 10*1024*1024;

	ParsingContext										parsing;

	ResolvedRoute										resolved;

	RequestBody											body;

	CGIContext											cgi;

	CgiHandler*											cgi_handler; // owns the live CGI child while one is running (NULL otherwise)

	bool												headers_only; // HEAD method
	bool												requires_CGI;
	bool												is_multipart;
	bool												body_chunked;
	bool												created_file;

	const Method&										getMethod(void) const;

	const std::string&									getPath(void) const;
	const std::string&									getQuery(void) const;
	const std::string&									getVersion(void) const;
	const std::string*									getHeader(const std::string& key) const;
	const std::map<std::string, std::string>&			getHeaders(void) const;
	const std::string*									getCookie(const std::string& key) const;

	const std::stringstream&							getBody(void) const;

	void												setMethod(const Method& method);
	void												setPath(const std::string&);
	void												setQuery(const std::string&);
	void												setVersion(const std::string&);
	void												setHeader(const std::string& key, const std::string& value);
	void												setCookie(const Cookie& cookie);

	bool												extractContentLength(void);
	bool												extractSessionID(void);

	void												reset(void);

private:

	HTTPRequest(const HTTPRequest& other);
	HTTPRequest& operator = (const HTTPRequest& other);

	Method												_method;

	std::string											_path;
	std::string											_query;
	std::string											_version;

	std::map<std::string,std::string>					_headers;

	std::vector<Cookie>									_cookies;

	std::string											_session_id;

};

#endif
