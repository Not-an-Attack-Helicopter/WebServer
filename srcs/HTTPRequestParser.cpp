/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequestParser.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sholz, bstorck <marvin@42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 18:35:38 by bstorck           #+#    #+#             */
/*   Updated: 2026/08/24 21:59:05 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/HTTPRequestParser.hpp"
#include "../incs/HTTPContentDisposition.hpp"
#include "../incs/HTTPContentType.hpp"
// #include "../incs/Dispatcher.hpp"
#include "../incs/HTTPGrammar.hpp"
#include "../incs/templates.hpp"
#include "../incs/constexpr.hpp"
#include "../incs/Config.hpp"
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
// #include <algorithm>
// #include <stdexcept>
#include <iostream>
// #include <fstream>
// #include <sstream>
#include <vector>
#include <cstddef>
// #include <climits>
#include <cctype>

/*
static int createFile(const Config::Location& location,
					  const std::string& path,
					  HTTPRequest& request) {

	log.debug("absolute path: " + path);

	int fd = -1;
	unsigned short count = 0;
	std::time_t timestamp = std::time(NULL);
	do {
		std::string unique_id = i2a(timestamp) + "-" + randomHexString(8);
		std::string file_path = path + location.upload_dir + "/.upload_" + unique_id + ".tmp";
		request.body.file_path = file_path;
		log.debug("file_path: " + file_path);
		fd = open(file_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
	} while (fd == -1 && errno == EEXIST && ++count < 11);

	return fd;

}
	request.body.file_fd = createFile(location, path, request);
*/

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

// unsigned long Parser::global_count = 0;

/*	@brief Instance	*/
Parser& Parser::instance(void) {
	static Parser instance;
	return instance;
}

// const Location* Parser::matchLocation(const std::vector<Location>& locations,
// 									  const std::string& location_path) {
//
// 	// Look for exact match
// 	for (size_t i = 0; i < locations.size(); ++i) {
// 		if (location_path == locations[i].path) {
// 			return &locations[i];
// 		}
// 	}
//
// 	// Longest prefix match wins
// 	const Location*	matched_location = NULL;
// 	size_t matched_location_path_len = 0;
//
// 	for (size_t i = 0; i < locations.size(); ++i) {
//
// 		const Location* config_location = &locations[i];
// 		std::string config_location_path = config_location->path;
// 		size_t config_location_path_len = config_location_path.length();
// 		size_t location_path_len = location_path.length();
// 		// log.error(config_location_path + " " + i2a(config_location_path_len));
// 		// log.error(location_path + " " + i2a(location_path_len));
// 		if (startsWith(location_path,
// 			config_location_path,
// 			location_path_len,
// 			config_location_path_len)) {
//
// 			// log.error("A config location matches with requested location.");
// 			config_location_path_len =	config_location_path.length();
// 			bool is_valid_boundary =	(config_location_path_len == location_path_len ||
// 										location_path[config_location_path_len] == '/' ||
// 										config_location_path == "/");
// 			// log.error(std::string("") + (requested_location_path[config_location_path_len]));
// 			// log.error(is_valid_boundary ? "valid boundary" : "invalid boundary");
// 			// log.error(i2a(config_location_path_len) + " vs " + i2a(matched_location_path_len));
//
// 			if (is_valid_boundary && config_location_path_len > matched_location_path_len) {
// 				matched_location_path_len = config_location_path_len;
// 				matched_location = config_location;
// 				// log.error("New match found! " + matched_location->path + " " + i2a(config_location_path_len));
// 			}
// 		}
// 	}
//
// 	// if (matched_location != NULL)
// 	// 	log.error("HERE > " + matched_location->path + " < HERE");
// 	return matched_location;
//
// }

// Feed raw bytes; returns the current parse_state:
// HTTPRequest::ParseState Parser::incomingData(const std::string& raw, HTTPRequest* request) {
bool Parser::buffer(Buffer& buffer, HTTPRequest& request) {

	// std::ostringstream oss;
	// for (size_t i = buffer.begin; i < buffer.end; ++i) {
	// 	oss << /*"byte[" << i << "]*/ "{" << (int)(unsigned char)buffer.data[i] << "} ";
	// }
	// log.error(oss.str());
// DEBUG BEGIN
	// log.notice("\n#################################################\n");
	// log.debug("request id: " + i2a(HR_object_id) + "\tstate: " + i2a(parse_state) + "\tparses: " + i2a(parses_count));
	// ++parses_count;
// DEBUG END

	// std::vector<char>::const_iterator begin = pos2it(buffer.data, buffer.begin);
	// std::vector<char>::const_iterator end = pos2it(buffer.data, buffer.end);

	// log.error("Parser: processing " + request.debug);

	switch (request.parsing.state) {

	case HTTPRequest::READING_REQUEST_LINE:
		return _parseRequestLine(buffer, request);
	case HTTPRequest::READING_HEADERS:
		return _parseHeaders(buffer, request);
	case HTTPRequest::READING_BODY:
		if (request.body_chunked) {
			// size_t bytes_decoded = _decode(buffer, request.decoded);
			// _parseBody(request.decoded, request);
			// request.parsing.bytes_read_count += bytes_decoded;
			// return request.parsing.bytes_read_count;
			return _parseChunks(buffer, request);
		} else {
			return _parseBody(buffer, request);
		}
	// case HTTPRequest::COMPLETE:
	// 	return false;
	// case HTTPRequest::ERROR:
	// 	return false;
	default:
		return false;

	}

}

Method Parser::extractMethod(const std::string& method) {

	static const std::string valid_methods[
		static_cast<int>(METHOD_COUNT)
	] = {
		"GET", "HEAD", "DELETE", "POST", "PUT"
	};
	for (size_t i = 0; i < static_cast<int>(METHOD_COUNT); ++i) {
		// log.error(valid_methods[i]);
		if (valid_methods[i] == method) {
			// log.error("SET METHOD: " + valid_methods[i]);
			// _method = static_cast<Method>(i);
			// return;
			return static_cast<Method>(i);
		}
	}
	// log.error("OH NO! NO METHOD!! WHAT SHOULD WE DO??");
	return METHOD_COUNT;

}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Constructor	*/
Parser::Parser(void) {
	log.debug("Parser Constructor called");
	return;
};

/*	@brief Copy Constructor	*/
Parser::Parser(const Parser& other)/* : _configs(other._configs) */{
	log.debug("Parser Copy Constructor called");
	*this = other;
	return;
};

/*	@brief Copy Assignment Operator	*/
Parser& Parser::operator=(const Parser& other) {
	if (this != &other) {
		log.debug("Parser Copy Assignment Operator called");
		// this->_configs = other._configs;
	}
	return *this;
};

/*	@brief Deconstructor	*/
Parser::~Parser() {
	log.debug("Parser Deconstructor called");
	return;
};

// size_t Parser::_findRequestLineEnd(const Client::Buffer& buffer, HTTPRequest& request) {
//
// 	// size_t LF_pos = raw.find(http::LF);
// 	std::vector<char>::const_iterator begin = pos2it(buffer.data, buffer.begin);
// 	std::vector<char>::const_iterator end = pos2it(buffer.data, buffer.end);
// 	std::vector<char>::const_iterator it = std::find(begin, end, http::LF);
// 	size_t LF_pos = std::string::npos;
// 	if (it != end) LF_pos = std::distance(begin, it);
//
// 	// log.error(i2a(LF_pos));
// 	// size_t CRLF_pos = raw.find(http::CRLF);
// 	std::string CRLF = http::CRLF;
// 	it = std::search(begin, end, CRLF.begin(), CRLF.end(), std::equal_to<char>());
// 	size_t CRLF_pos = std::string::npos;
// 	if (it != end) CRLF_pos = std::distance(begin, it);
// 	// log.error(i2a(CRLF_pos));
//
// 	// Both not found: incomplete data
// 	if (LF_pos == std::string::npos && CRLF_pos == std::string::npos)
// 		return std::string::npos;
//
// 	// Determine which style to use
// 	if (LF_pos < CRLF_pos) // true if unix style (win = npos) / false if windows style
// 		return LF_pos;
//
// 	// _is_unix_style = false;
// 	request.parsing.line_ending = IS_CRLF;
// 	return CRLF_pos;
//
// }

size_t Parser::_findRequestLineEnd(const Buffer& buffer, HTTPRequest& request) {

	ssize_t LF_pos = buffer.find(http::LF);
	if (LF_pos == -1) return std::string::npos;
	// if (LF_pos > HTTPRequest::MAX_REQUEST_LINE_LENGTH)
	// 	return HTTPRequest::MAX_REQUEST_LINE_LENGTH;
	if (LF_pos != 0 && buffer.data[LF_pos - 1] == http::CR) {
		request.parsing.line_ending = HTTPRequest::CRLF;
		// log.error("IS_CRLF");
		// log.error(request.parsing.line_ending == HTTPRequest::CRLF ? "Yep, confirmed" : "Oh oh, something unexpected happened");
		return LF_pos - 1;
	}
	// log.error("IS_LF");
	// log.error(request.parsing.line_ending == HTTPRequest::LF ? "Yep, confirmed" : "Oh oh, something unexpected happened");
	return LF_pos;

}

// bool Parser::_matchMethod(const std::string& method) {
//
// 	static const std::string valid_methods[5] = {"GET", "HEAD", "DELETE", "POST", "PUT"};
// 	for (size_t i = 0; i < arraySize(valid_methods); ++i) {
// 		if (valid_methods[i] == method) {
// 			// _method = static_cast<Method>(i);
//
// 			return true;
// 		}
// 	}
// 	return false;
//
// }

bool Parser::_extractTokens(const Buffer& buffer, HTTPRequest& request) {

	// Transform to stream
	// std::stringstream ss(line);
	std::stringstream ss;
	buffer.sstream(ss, 0, request.parsing.line_end_pos);
	if (ss.fail()) {
		log.error("parse error: failed to set stringstream content");
		request.parsing.error_cause = INTERNAL_SERVER_ERROR;
		return false;
	}
	// log.debug(ss.str());

	// Validate number of tokens
	std::string method, target, version, extra;
	if (!(ss >> method >> target >> version)) {
		log.warn("request line: not enoug tokens found");
		request.parsing.error_cause = BAD_REQUEST;
		return false; // too few tokens in request line
	}
	if (ss >> extra) {
		log.warn("request line: too many tokens found");
		request.parsing.error_cause = BAD_REQUEST;
		return false; // too many tokens in request line
	}

	// Validate method
	// if (!isSupportedMethod(method)) {
	// 	log.error("Bad method");
	// 	return false;
	// }
	const Method m = extractMethod(method);
	if (m >= METHOD_COUNT) {
		log.warn("request line: unknown method");
		request.parsing.error_cause = BAD_REQUEST;
	}
	// 	return false;
	// } else {
	// 	// log.error("Good method");
	// 	request.setMethod(m);
	// 	// request.setMethod(extractMethod(method));
	// 	// log.error(i2a(request.getMethod()) + " " + request.getMethodName());
	// }
	if (m == HEAD) {
		request.headers_only = true;
	}
	request.setMethod(m);
	// log.error("Good method");
	// Method meth = extractMethod(method);
	// if (meth == METHOD_COUNT) {
	// 	log.error("Bad method");
	// 	return false;
	// }
	// log.error("Still good method");
	// request.setMethod(extractMethod(method));
	// log.error(i2a(request.getMethod()) + " " + request.getMethodName());

	// Validate target
	if (target.empty() || target[0] != '/') {
		log.warn("request line: invalid target");
		request.parsing.error_cause = BAD_REQUEST;
		return false;
	}

	// Split target into path and query
	size_t query_start_pos = target.find('?');
	if (query_start_pos != std::string::npos) {
		request.setPath(target.substr(0, query_start_pos));
		request.setQuery(target.substr(query_start_pos + 1));
	} else {
		request.setPath(target);
		request.setQuery("");
	}

	// Validate HTTP version
	version = trim(version); // strip trailing \r
	if (version != http::V_1_1 && version != http::V_1_0) {
	// if (version.substr(0, 4) != "HTTP")
	// if (std::strncmp(version.c_str(), "HTTP", 4) != 0)
		log.warn("request line: http version not supported");
		request.parsing.error_cause = HTTP_VERSION_NOT_SUPPORTED;
		return false;
	}
	request.setVersion(version);

	return true;

}

bool Parser::_parseHeaderLine(const Buffer& buffer, HTTPRequest& request) {

	// log.error(&buffer.data[buffer.begin]);
	// request.parsing.header_line_size = 0;
	// log.error(i2a(request.parsing.header_line_start_pos) + " - " + i2a(request.parsing.header_line_end_pos));
	// std::string buff = buffer.substr(request.parsing.header_line_start_pos,
									 // request.parsing.header_line_end_pos);
	// log.error(buff);
	std::ostringstream oss;
	// Find separator ':'
	size_t colon_pos = static_cast<size_t>(buffer.find(':'));
	// size_t colon_pos = buff.find(':');
	if (colon_pos == 0) {
		log.warn("request: header field has no field-name");
		// request.parsing.error_cause = BAD_REQUEST;
		return false;
	}
	if (colon_pos == request.parsing.line_end_pos || colon_pos == std::string::npos) {
		log.warn("request: header field has no colon");
		// request.parsing.error_cause = BAD_REQUEST;
		return false;
	}
	// log.debug("colon found at pos " + i2a(colon_pos));
	if (request.parsing.state == HTTPRequest::READING_HEADERS) {
		for (size_t i = 0; i < colon_pos; ++i) {
			oss << "{" << (int)(unsigned char)buffer.data[buffer.begin + i] << "} ";
			// oss << "{" << (int)(unsigned char)buff[i] << "} ";
			if (!HTTPGrammar::isTChar(buffer.data[buffer.begin + i])) {
			// if (!isTchar(buff[i])) {
				log.warn("request header: invalid character or whitespace in field-name " + oss.str());
				// request.parsing.error_cause = BAD_REQUEST;
				return false;
			}
		}
		// log.debug(oss.str());
	}
	oss.clear();

	// log.error(i2a(colon_pos));
	// log.error(buffer.str());
	std::string key = buffer.substr(0, colon_pos);
	// std::string key = buff.substr(0, colon_pos);
	// Lowercase key for case-insensitive lookup
	for (std::string::iterator it = key.begin(); it != key.end(); ++it) {
		*it = std::tolower(static_cast<unsigned char>(*it));
	}
	// log.debug("added key: " + key);
	// if (key.find_first_of(" \t\r\n") != std::string::npos) {
	// 	log.error("parse error: invalid whitespace in field-name");
	// 	return false;
	// }
	// // Trim whitespaces from key
	// // key = trim(key);
	// // if (key.empty()) {
	// // 	log.error("parse error: empty key provided");
	// // 	return false; // key is required by header
	// // }

	// size_t line_end =	request.parsing.line_ending == CRLF ?
	// 					// buffer.find(http::CRLF) :
	// 					buff.find(http::CRLF) :
	// 					// buffer.find(http::LF);
	// 					buff.find(http::LF);
	// // std::string value = buffer.substr(colon_pos + 1, line_end);
	// std::string value = buff.substr(colon_pos + 1, line_end);
	std::string value = buffer.substr(colon_pos + 1, request.parsing.line_end_pos);
	// log.error(value);
	// Trim whitespaces from value
	value = trim(value);
	// log.error(value);
	for (size_t i = 0; i < value.size(); ++i) {
		oss << "{" << (int)(unsigned char)value[i] << "}";
		unsigned char c = static_cast<unsigned char>(value[i]);
		if ((c < 0x20 && c != '\t') || c == 0x7f) {
			log.warn("request header: corrupt data in field-value");
			log.warn("culprit: {" + i2a((int)(unsigned char)value[i]) + "}");
			// request.parsing.error_cause = BAD_REQUEST;
			return false;
		}
	}
	// log.debug(oss.str());

	if (request.parsing.state == HTTPRequest::READING_HEADERS) {
		// request._headers[key] = value;
		request.setHeader(key, value);
	} else if (request.parsing.state == HTTPRequest::READING_BODY) {
		if (request.parsing.chunk_state ==  HTTPRequest::READING_TRAILERS) {
			// request.body._trailers[key] = value;
			request.body.setHeader(key, value);
		} else {
			// request.body.parts.back().headers[key] = value;
			request.body.parts.back().setHeader(key, value);
		}
	}
	log.debug("added header: " + key + ": " + value);

	return true;

}

// HTTPRequest::ParseState Parser::_parseRequestLine(const std::string& raw, HTTPRequest& request) {
bool Parser::_parseRequestLine(const Buffer& buffer, HTTPRequest& request) {

	// Find where request line ends
	request.parsing.line_end_pos = _findRequestLineEnd(buffer, request);
	// log.error(&buffer.data[buffer.begin]);
	// log.debug("request_line_end_pos: " + i2a(request.parsing.line_end_pos));

	// Calculate headers start position based on line ending style ("\n" or "\r\n")
	if (request.parsing.line_ending == HTTPRequest::CRLF) {
		request.parsing.line_end_size = CRLF_SIZE;
		request.parsing.blank_line_size = CRLFCRLF_SIZE;
	} else {
		request.parsing.line_end_size = LF_SIZE;
		request.parsing.blank_line_size = CRLF_SIZE;
	}
	// log.debug("line_end_size: " + i2a(request.parsing.line_end_size));

	// Detected empty line (before start of request line): not copied into buffer
	if (request.parsing.line_end_pos == 0) {

		 // Set byte count to line_end_size to drop from data
		request.parsing.bytes_read_count = request.parsing.line_end_size;
		// log.debug("bytes_read: " + i2a(request.parsing.bytes_read_count));
		// return request.parsing.state;
		return true;

	// No line feed detected (Data only): wait for more data
	} else if (request.parsing.line_end_pos == std::string::npos) {

		// request.parsing.buffer.append(raw);

		// request.parsing.bytes_read_count = request.parsing.buffer.size() - request.parsing.old_buffer_fill_level;
		// request.parsing.bytes_read_count = buffer.end - buffer.begin;
		// request.parsing.bytes_read_count = buffer.range();
		request.parsing.bytes_read_count = std::string::npos;
		log.debug("bytes_read: " + i2a(request.parsing.bytes_read_count));

		// log.debug("Bytes read: " + i2a(bytes_read_count));
		// log.debug("Previous buffer fill level: " + i2a(old_buffer_fill_level));

		// request.parsing.old_buffer_fill_level = request.parsing.buffer.size();

		// log.debug("Current buffer fill level: " + i2a(buffer.size()));
		// log.debug("Current data in buffer (request):\n");
		// log.notice((buffer));

		// return request.parsing.state;
		return false;

	// Maximum request line length exceeded
	} else if (request.parsing.line_end_pos > HTTPRequest::MAX_REQUEST_LINE_LENGTH) {
		log.warn("parse error: maximum request line length exceeded");
		request.parsing.error_cause = URI_TOO_LONG;
		request.parsing.state = HTTPRequest::ERROR;
		return false;

	// Line feed detected (end of request line): procced with line parsing
	} else {

		// request.parsing.buffer.append(raw, 0, request.parsing.request_line_end_pos + request.parsing.line_end_size);
		// // buffer.append(::LF); DO NOT APPEND LINE FEED!

		// request.parsing.bytes_read_count = request.parsing.buffer.size() - request.parsing.old_buffer_fill_level;
		// log.debug("Bytes read: " + i2a(bytes_read_count));
		// log.debug("Previous buffer fill level: " + i2a(old_buffer_fill_level));

		request.parsing.bytes_read_count =	request.parsing.line_end_pos +
											request.parsing.line_end_size;
		log.debug("bytes_read: " + i2a(request.parsing.bytes_read_count));

		// request.parsing.old_buffer_fill_level = request.parsing.buffer.size();

		// log.debug("Current buffer fill level: " + i2a(buffer.size()));
		// log.debug("Current data in buffer (request):\n");
		// log.notice((buffer));
		// log.debug("Line: " + buffer);

		// Extract method, path, query, and version
		// if (!_extractTokens(request.parsing.buffer, request)) {
		// std::string line = buffer.substr(0, request.parsing.request_line_end_pos + request.parsing.line_end_size);
		// std::stringstream ss = buffer.sstream(0, request.parsing.request_line_end_pos + request.parsing.line_end_size);
		if (!_extractTokens(buffer, request)) {
			// log.error("something went wrong while parsing a request line");
			request.parsing.state = HTTPRequest::ERROR;
			// return request.parsing.state;
			return false;
		}

		request.parsing.state = HTTPRequest::READING_HEADERS;
		// return request.parsing.state;
		return true;
	}

}

bool Parser::_parseHeaders(const Buffer& buffer, HTTPRequest& request) {

	// Check for line break
	// request.parsing.line_end_pos = request.parsing.line_ending == CRLF ?
	// 							   buffer.find(http::CRLF) :
	// 							   buffer.find(http::LF);
	if (request.parsing.line_ending == HTTPRequest::CRLF) {
		request.parsing.line_end_pos = buffer.find(http::CRLF);
	} else {
		request.parsing.line_end_pos = buffer.find(http::LF);
	}
	// log.error(&buffer.data[buffer.begin]);
	// log.debug("header_line_end_pos: " + i2a(request.parsing.line_end_pos));

	// Empty line detected, proceed with validity checks
	if (request.parsing.line_end_pos == 0) {

		// log.debug("CRLF = 0, CRLFCRLF = 0");
		log.debug("empty line detected");
		log.error(buffer.str());
		request.parsing.bytes_read_count = request.parsing.line_end_size;
		// log.debug("bytes read: " + i2a(request.parsing.bytes_read_count));

		if (request.parsing.state == HTTPRequest::READING_HEADERS) {

			// const std::map<std::string, std::string>& headers = request.getHeaders(0);

			const std::string* content_disposition = request.getHeader("content-disposition");
			if (content_disposition !=  NULL) {
				log.error("HTTP Content-Disposition: [" + *content_disposition + "]");
				if (!extractContentDisposition(*content_disposition,
											   HTTPContentDisposition::HTTP,
											   request.body.disposition,
											   &request.body.name,
											   &request.body.filename,
											   &request.body.filenameStar,
											   &request.body.d_parameters)) {
					log.warn("request: invalid content-disposition header provided");
				}
			// 	const std::string filename = request.body.filename;
			// 	if (!filename.empty()) {
			// 		log.error("filename: " + filename);
			// 		request.body.extension = extraxtExtension(filename);
			// 		log.error("extension: " + request.body.extension);
			// 	}
			// } else {
			// 	log.error("HTTP CONTENT-DISPOSITION NULL");
			}


			// if (request.hasHeader("content-type")) {
			// if (includesHeader(headers, "content-type")) {
			// 	const std::string& type = request.getHeader("content-type", 0);
			const std::string* type = request.getHeader("content-type");
			if (type != NULL) {
				if (!HTTPContentType::extractContentType(*type, request.body.type,
														 &request.body.boundary,
														 &request.body.t_parameters)) {
					log.warn("request: invalid content-type header provided");
				}
				if (HTTPGrammar::equalCI(request.body.type, "multipart/form-data")) {
					request.is_multipart = true;
					log.error(request.body.type);
					log.error("IS MULTIPART!");
				}
			}

			const std::string* transfer_endcoding = request.getHeader("transfer-encoding");
			if (transfer_endcoding != NULL && HTTPGrammar::equalCI(*transfer_endcoding, "chunked")) {
				request.body_chunked = true;
				log.error("IS CHUNKED!");
			}

			// Check for Host Header (mandatory for HTTP/1.1)
			if (request.getVersion() == http::V_1_1 && request.getHeader("host") == NULL) {
				// (!request.hasHeader("host") || request.getHeader("host").empty())) {
				// (!includesHeader(headers, "host") || request.getHeader("host", 0).empty())) {
				log.warn("request: no host header provided");
				request.parsing.error_cause = BAD_REQUEST;
				request.parsing.state = HTTPRequest::ERROR;
				return false;
			}

			// GET and HEAD are not designed to carry a request body
			const Method requested_method = request.getMethod();
			if (requested_method == GET || requested_method == HEAD) {
				request.parsing.state = HTTPRequest::DISPATCHING;
				return true;
			}

			// DELETE may come with a body (optional)
			if (requested_method == DELETE) {
				// if (request.hasHeader("content-length")) {
				// if (includesHeader(headers, "content-length")) {
				if (request.getHeader("content_length") !=  NULL) {
					if (!request.extractContentLength()) {
						log.info("request: no content-length header provided");
						request.body.size = 0;
					}
				} else {
					request.parsing.state = HTTPRequest::DISPATCHING;
					return true;
				}
			}

			// Extract Content-Length value (mandatory for POST and PUY)
			if ((requested_method == POST || requested_method == PUT) &&
				!request.body_chunked && !request.extractContentLength()) {
				log.warn("request: no content-length header provided");
				request.parsing.error_cause = LENGTH_REQUIRED;
				request.parsing.state = HTTPRequest::ERROR;
				return false;
			}

			// Check Content-Length value against global treshold
			// if (request.parsing.content_length > HTTPRequest::SERVER_MAX_BODY_SIZE) {
			if (request.body.size > Config::SERVER_MAX_BODY_SIZE) {
				log.warn("request: content-length exceeds global treshold");
				request.parsing.error_cause = PAYLOAD_TOO_LARGE;
				request.parsing.state = HTTPRequest::ERROR;
				return false;
			}

			request.parsing.state = HTTPRequest::DISPATCHING;
			// request.parsing.state = HTTPRequest::READING_BODY;
			// request.parsing.state = HTTPRequest::COMPLETE;
			// return request.parsing.state;
			// return true;

			// request.parsing.state = HTTPRequest::ERROR;
			// return false;

		} else if (request.parsing.state == HTTPRequest::READING_BODY) {

			if (request.parsing.chunk_state == HTTPRequest::READING_TRAILERS) {

				log.error("END OF TRAILERS");
				request.parsing.chunk_state = HTTPRequest::END_OF_CHUNKS;
				log.error("END OF CHUNKS");
				promoteFile(request);
				request.parsing.state = HTTPRequest::COMPLETE;

			} else {

				// std::string							disposition;	✓
				// std::string							filename;		✓
				// std::string							name;			✓
				// std::string							type;			✓

				// const std::map<std::string, std::string>& headers = request.getHeaders();

				// if (includesHeader(headers, "content-disposition")) {
				// 	if (!request.extractContentDisposition()) {
				const std::string* content_disposition = request.body.parts.back().getHeader("content-disposition");
				if (content_disposition != NULL) {
					log.error("PART Content-Disposition: [" + *content_disposition + "]");
					// log.error("DING");
					if (!extractContentDisposition(*content_disposition,
												HTTPContentDisposition::MULTIPART_FORM_DATA,
												request.body.parts.back().disposition,
												&request.body.parts.back().name,
												&request.body.parts.back().filename)) {
						log.warn("request: invalid content-disposition part header provided");
					}
				// 	const std::string filename = request.body.parts.back().filename;
				// 	if (!filename.empty()) {
				// 		log.error("filename: " + filename);
				// 		request.body.parts.back().extension = extraxtExtension(filename);
				// 		log.error("extension: " + request.body.parts.back().extension);
				// 	}
				// } else {
				// 	log.error("PART CONTENT-DISPOSITION NULL");
				}


				// if (includesHeader(headers, "content-type")) {
					// if (!request.extractContentType(true)) {
				const std::string* type = request.getHeader("content-type");
				if (type != NULL) {
					// log.error("DONG");
					if (!HTTPContentType::extractContentType(*type, request.body.parts.back().type,
															NULL, &request.body.parts.back().t_parameters)) {
						log.warn("request: invalid content-type part header provided");
					}
				}

				request.parsing.multipart_state = HTTPRequest::READING_PART_BODY;
				if (!createFile(request)) {
					log.error("error on file creation");
					request.parsing.state =  HTTPRequest::ERROR;
					request.parsing.error_cause = PAYLOAD_TOO_LARGE;
					return false;
				}

			}

		}

		log.error(buffer.str());
		return true;

	// No line break detected, wait for more data
	} else if (request.parsing.line_end_pos == std::string::npos) {

		// log.debug("CRLF = npos, CRLFCRLF = ???");
		log.debug("waiting for more data...");
		// request.parsing.bytes_read_count = buffer.range();
		request.parsing.bytes_read_count = std::string::npos;
		// log.debug("bytes read: " + i2a(request.parsing.bytes_read_count));
		return false;

	// Maximum header line length exceeded
	} else if (request.parsing.line_end_pos > HTTPRequest::MAX_HEADER_LINE_LENGTH) {
		log.warn("request: maximum header line length exceeded");
		request.parsing.error_cause = REQUEST_HEADER_FIELDS_TOO_LARGE;
		request.parsing.state = HTTPRequest::ERROR;
		return false;

	// Detected line break, parse line
	} else {

		// log.debug("CRLF = " + i2a(request.parsing.line_end_pos) + ", CRLFCRLF = ???");
		// log.debug("parsing header line...");

		request.parsing.headers_size += request.parsing.line_end_pos;
		// log.debug("headers_size: " + i2a(request.parsing.headers_size));
		if (request.parsing.headers_size > HTTPRequest::MAX_TOTAL_HEADERS_SIZE) {
			log.warn("request: total header size exceeds the maximum allowed");
			request.parsing.error_cause = REQUEST_HEADER_FIELDS_TOO_LARGE;
			request.parsing.state = HTTPRequest::ERROR;
			return false;
		}

		if (!_parseHeaderLine(buffer, request)) {
			log.error("parse error: something went wrong while parsing a header line");
			request.parsing.error_cause = BAD_REQUEST;
			request.parsing.state = HTTPRequest::ERROR;
			return false;
		}

		request.parsing.bytes_read_count = request.parsing.line_end_pos + request.parsing.line_end_size;
		log.debug("bytes read: " + i2a(request.parsing.bytes_read_count));
		// log.error(buffer.str());
		return true;

	}

}

		// request.parsing.header_line_start_pos	= 0;
		// request.parsing.header_line_end_pos		= 	request.parsing.line_ending == CRLF ?
		// 											buffer.find(http::CRLF) :
		// 											buffer.find(http::LF);;

		// log.error("header_line_end_pos = " + i2a(request.parsing.header_line_end_pos));
		// while (request.parsing.header_line_end_pos <= request.parsing.headers_end_pos) {
  //
		// 	if (!_parseHeaderLine(buffer, request)) {
		// 		log.error("something went wrong while parsing a header line");
		// 		request.parsing.state = HTTPRequest::ERROR;
		// 		return false;
		// 	}
  //
		// 	request.parsing.header_line_start_pos = request.parsing.header_line_end_pos + request.parsing.line_end_size;
		// 	if (request.parsing.header_line_start_pos >= request.parsing.headers_end_pos) break;
		// 	size_t i = request.parsing.header_line_start_pos;
		// 	while (++i < request.parsing.headers_end_pos) {
		// 		request.parsing.header_line_end_pos = i;
		// 		if (buffer.data[buffer.begin + i] == '\r') break;
		// 	}
  //
		// }
			// if (request.parsing.header_line_end_pos == 0) {
			// 	log.error("CRLFCRLF = npos, CRLF = 0");
			// 	log.error("something went wrong while searching for headers");
			// 	request.parsing.state = HTTPRequest::COMPLETE;
			// 	return true;
   //
			// } else if (request.parsing.header_line_end_pos == std::string::npos) {
			// 	request.parsing.bytes_read_count = buffer.range();
			// 	log.error("CRLFCRLF = npos, CRLF = npos");
			// 	log.debug("Bytes read: " + i2a(request.parsing.bytes_read_count));
			// 	return false;
   //
			// } else {
   //
			// 	if (!_parseHeaderLine(buffer, request)) {
			// 		log.error("something went wrong while parsing a header line");
			// 		request.parsing.state = HTTPRequest::ERROR;
			// 		return false;
			// 	}
   //
			// 	request.parsing.bytes_read_count = request.parsing.header_line_end_pos;
			// 	log.debug("Bytes read: " + i2a(request.parsing.bytes_read_count));
			// 	log.error("CRLFCRLF = npos, CRLF = " + i2a(request.parsing.bytes_read_count) + " bytes");
			// 	return true;
   //
			// }

			// log.error("CRLFCRLF = N bytes, CRLF = ???");
			// request.parsing.bytes_read_count = request.parsing.headers_end_pos + request.parsing.blank_line_size;
			// log.debug("Bytes read: " + i2a(request.parsing.bytes_read_count));
			// // return true;
			// if (!_parseHeaderLine(buffer, request)) {
			// 	log.error("something went wrong while parsing a header line");
			// 	request.parsing.state = HTTPRequest::ERROR;
			// 	return false;
			// }

		// Calculate cumulative size of header lines
		// request.parsing.headers_start_pos =	request.parsing.request_line_end_pos +
		// 									request.parsing.line_end_size;
		// request.parsing.headers_size =		request.parsing.headers_end_pos -
		// 									request.parsing.headers_start_pos +
		// 									request.parsing.line_end_size;
  //
		// request.parsing.bytes_read_count =	request.parsing.headers_end_pos -
		// 									request.parsing.headers_start_pos;

		// log.error("headers_end_pos: " + i2a(request.parsing.headers_end_pos));
		// if (request.parsing.headers_end_pos != std::string::npos) {
		// 	// Print the 8 bytes around this position
		// 	for (size_t i = (request.parsing.headers_end_pos > 2 ? request.parsing.headers_end_pos - 2 : 0);
		// 		i < request.parsing.headers_end_pos + 6 && i < buffer.range(); ++i) {
		// 		std::cout	<< "byte[" << i << "] = " << (int)(unsigned char)buffer.data[buffer.begin + i]
		// 					<< std::endl;
		// 	}
		// }

		// // Check for Host Header (mandatory for HTTP/1.1)
		// if (request.getVersion() == http::V_1_1 &&
		// 	(!request.hasHeader("host") || request.getHeader("host").empty())) {
		// 	log.error("parse error: no host header found");
		// 	request.parsing.state = HTTPRequest::ERROR;
		// 	return false;
		// }
  //
		// // GET and DELETE are not designed to carry request bodies
		// if (request.getMethod() == GET || request.getMethod() == DELETE) {
		// 	request.parsing.state = HTTPRequest::COMPLETE;
		// 	return true;
		// }
  //
		// // Extract Content-Length value (mandatory for POST)
		// if (!request.extractContentLength()) {
		// 	log.error("parse error: no content-length header found");
		// 	request.parsing.state = HTTPRequest::ERROR;
		// 	return false;
		// }
  //
		// request.parsing.state = HTTPRequest::READING_BODY;
		// // return request.parsing.state;
		// return true;

// 	}

// }

	// 	// request.parsing.bytes_read_count = buffer.range();
	// 	// log.debug("Bytes read: " + i2a(request.parsing.bytes_read_count));
 //
	// 	// log.debug("no empty line in buffer \t\t yet");
	// 	// return false;
 //
 //
	// // // Detected line break at 0 pos (empty line): ~append and proceed with validity checks~
	// 	// if (request.parsing.header_line_end_pos == 0) {
 //
	// 	// request.parsing.buffer.append(raw, 0, request.parsing.line_end_size);
 //
	// 	// request.parsing.bytes_read_count = request.parsing.line_end_size;
 //
	// 	// log.debug("Bytes read: " + i2a(request.parsing.bytes_read_count));
	// 	// log.debug("Previous buffer fill level: " + i2a(request.parsing.old_buffer_fill_level));
 //
	// 	// request.parsing.old_buffer_fill_level += request.parsing.line_end_size;
 //
	// 	// log.debug("Current buffer fill level: " + i2a(request.parsing.buffer.size()));
	// 	// log.debug("Current data in buffer (request):\n");
	// 	// log.notice((request.parsing.buffer));
 //
	// 	// // return parse_state; DO NOT RETURN AT THIS LINE!
 //
	// 	// After detecting empty line, check buffer for end of headers
	// 	// request.parsing.headers_end_pos =	request.parsing.line_ending == IS_CRLF ?
	// 	// 									request.parsing.buffer.find(http::CRLFCRLF) :
	// 	// 									request.parsing.buffer.find(http::LFLF);
	// 	// log.error("headers_end_pos: " + i2a(request.parsing.headers_end_pos));
	// 	// if (request.parsing.headers_end_pos != std::string::npos) {
	// 	// 	// Print the 8 bytes around this position
	// 	// 	for (size_t i = (request.parsing.headers_end_pos > 2 ? request.parsing.headers_end_pos - 2 : 0);
	// 	// 		 i < request.parsing.headers_end_pos + 6 && i < buffer.range(); ++i) {
	// 	// 		std::cout	<< "byte[" << i << "] = " << (int)(unsigned char)buffer.data[buffer.begin + i]
	// 	// 					<< std::endl;
	// 	// 	}
	// 	// }
 //
	// 	// ~Detected empty line (before start of request line): invalid request~
	// 	// ~(should never happen)~
	// 	// Detected empty line (before any header line): invalid request
	// 	if (request.parsing.headers_end_pos == 0) {
 //
	// 		// log.error("YOU WANT TO READ THIS LINE!");
	// 		log.error("parse error: unexpected empty line");
	// 		request.parsing.state = HTTPRequest::ERROR;
	// 		// return request.parsing.state;
	// 		return false;
 //
	// 	// No empty line detected: expecting more header lines
	// 	// ~(should not occur: appended line break right before check)~
	// 	} else if (request.parsing.headers_end_pos == std::string::npos) {
 //
	// 		request.parsing.bytes_read_count = buffer.range();
	// 		log.debug("Bytes read: " + i2a(request.parsing.bytes_read_count));
 //
	// 		// log.debug("no empty line in buffer \t\t yet");
	// 		return false;
 //
	// 	// Detected empty line: end of header lines
	// 	} else {
 //
	// 		// Set size of empty line based on line ending style ("\n\n" or "\r\n\r\n")
	// 		request.parsing.blank_line_size = request.parsing.line_ending == CRLF ? CRLFCRLF_SIZE : LFLF_SIZE;
 //
	// 		request.parsing.bytes_read_count = request.parsing.blank_line_size;
 //
	// 		// Calculate cumulative size of header lines
	// 		request.parsing.headers_start_pos =	request.parsing.request_line_end_pos +
	// 											request.parsing.line_end_size;
	// 		request.parsing.headers_size =		request.parsing.headers_end_pos -
	// 											request.parsing.headers_start_pos +
	// 											request.parsing.line_end_size;
 //
	// 		// Check for Host Header (mandatory for HTTP/1.1)
	// 		if (request.getVersion() == http::V_1_1 &&
	// 			(!request.hasHeader("host") || request.getHeader("host").empty())) {
	// 			log.error("parse error: no host header found");
	// 			request.parsing.state = HTTPRequest::ERROR;
	// 			return request.parsing.state;
	// 		}
 //
	// 		// GET and DELETE are not designed to carry request bodies
	// 		if (request.getMethod() == GET || request.getMethod() == DELETE) {
	// 			request.parsing.state = HTTPRequest::COMPLETE;
	// 			return request.parsing.state;
	// 		}
 //
	// 		// Extract Content-Length value (mandatory for POST)
	// 		if (!request.extractContentLength()) {
	// 			log.error("parse error: no content-length header found");
	// 			request.parsing.state = HTTPRequest::ERROR;
	// 			return request.parsing.state;
	// 		}
 //
	// 		request.parsing.state = HTTPRequest::READING_BODY;
	// 		return request.parsing.state;
 //
	// 	}
 //
	// // No line break: wait for more data
	// } else if (request.parsing.header_line_end_pos == std::string::npos) {
 //
	// 	log.error("npos detected");
	// 	// request.parsing.buffer.append(raw);
 //
	// 	// request.parsing.bytes_read_count = request.parsing.buffer.size() - request.parsing.old_buffer_fill_level;
	// 	request.parsing.bytes_read_count = buffer.end - buffer.begin;
	// 	request.parsing.header_line_size += request.parsing.bytes_read_count;
 //
	// 	log.debug("Bytes read: " + i2a(request.parsing.bytes_read_count));
	// 	log.debug("Line length: " + i2a(request.parsing.header_line_size));
	// 	// log.debug("Previous buffer fill level: " + i2a(request.parsing.old_buffer_fill_level));
 //
	// 	// request.parsing.old_buffer_fill_level = request.parsing.buffer.size();
 //
	// 	// log.debug("Current buffer fill level: " + i2a(request.parsing.buffer.size()));
	// 	// log.debug("Current data in buffer (request):\n");
	// 	// log.notice((request.parsing.buffer));
 //
	// 	return request.parsing.state;
 //
	// // Data detected: proceed with line parsing
	// } else {
 //
	// 	// request.parsing.buffer.append(raw, 0, request.parsing.header_line_end_pos + request.parsing.line_end_size);
	// 	// // buffer.append(::LF); DO NOT APPEND LINE FEED!
 //
	// 	// request.parsing.bytes_read_count = request.parsing.buffer.size() - request.parsing.old_buffer_fill_level;
	// 	// request.parsing.bytes_read_count = request.parsing.header_line_end_pos + request.parsing.line_end_size;
	// 	request.parsing.bytes_read_count = request.parsing.header_line_end_pos;
	// 	request.parsing.header_line_size += request.parsing.bytes_read_count;
 //
	// 	log.debug("Bytes read: " + i2a(request.parsing.bytes_read_count));
	// 	log.debug("Line length: " + i2a(request.parsing.header_line_size));
	// 	// log.debug("Previous buffer fill level: " + i2a(request.parsing.old_buffer_fill_level));
 //
	// 	// request.parsing.old_buffer_fill_level = request.parsing.buffer.size();
 //
	// 	// log.debug("Current buffer fill level: " + i2a(request.parsing.buffer.size()));
	// 	// log.debug("Current data in buffer (request):\n");
	// 	// log.notice((request.parsing.buffer));
 //
	// 	// Parse header line
	// 	// std::string line = buffer.substr(buffer.size() - bytes_read_count);
	// 	// std::string line = request.parsing.buffer.substr(request.parsing.buffer.size() - request.parsing.header_line_size);
	// 	// log.error(line + i2a(header_line_size));
	// 	if (!_parseHeaderLine(buffer, request)) {
	// 		log.error("something went wrong while parsing a header line");
	// 		request.parsing.state = HTTPRequest::ERROR;
	// 		return request.parsing.state;
	// 	}
 //
	// 	return request.parsing.state;
 //
	// }

	// // After detecting empty line, check buffer for end of headers
	// _is_unix_style ?
	// 	headers_end_pos = buffer.find(LF ::LF) :
	// 	headers_end_pos = buffer.find(CRLF ::CRLF);
	//
	// // Detected empty line (before start of request line): invalid request
	// // (should never happen)
	// if (headers_end_pos == 0) {
	//
	// 	log.error("unexpected empty line");
	// 	bytes_read_count = 0;
	// 	parse_state = ERROR;
	// 	return parse_state;
	//
	// // No empty line detected: expecting more header lines
	// // (should not occur: return after line parsing)
	// } else if (headers_end_pos == std::string::npos) {
	//
	// 	// log.debug("no empty line in buffer\t\tyet");
	// 	return parse_state;
	//
	// // Detected empty line: end of header lines
	// } else {
	//
	// 	// Calculate cumulative size of header lines
	// 	headers_start_pos = request_line_end_pos + line_end_size;
	// 	headers_size = headers_end_pos - headers_start_pos + 1;
	//
	// 	// log.debug("request_line_end_pos: " + i2a(request_line_end_pos));
	// 	// log.debug("line_end_size: " + i2a(line_end_size));
	// 	// log.debug("headers_start_pos: " + i2a(headers_start_pos));
	// 	// log.debug("headers_end_pos: " + i2a(headers_end_pos));
	// 	// log.debug("headers_size: " + i2a(headers_size));
	//
	// 	// Check for Host Header (mandatory for GET, POST, DELETE)
	// 	if (!hasHeader("host") || getHeader("host").empty()) {
	// 		log.error("no host header found");
	// 		parse_state = ERROR;
	// 		return parse_state;
	// 	}
	//
	// 	// GET and DELETE are not designed to carry request bodies
	// 	if (_method == "GET" || _method == "DELETE") {
	// 		parse_state = COMPLETE;
	// 		return parse_state;
	// 	}
	//
	// 	// Extract Content-Length value (mandatory for POST)
	// 	if (!_extractContentLength()) {
	// 		log.error("no content-length header found");
	// 		parse_state = ERROR;
	// 		return parse_state;
	// 	}
	//
	// 	parse_state = READING_BODY;
	// 	return parse_state;
	//
	// }

// }

bool Parser::_parseChunks(Buffer& buffer, HTTPRequest& request) {

	HTTPRequest::ParsingContext& p = request.parsing;
	p.bytes_read_count = 0;

	switch (p.chunk_state) {

	case HTTPRequest::READING_SIZE: {

		log.error("READING SIZE");
		ssize_t pos;
		// ssize_t pos = buffer.find(p.line_ending);
		if (p.line_ending == HTTPRequest::CRLF) {
			log.error("looking for \\r\\n");
			pos = buffer.find(http::CRLF);
		} else if (p.line_ending == HTTPRequest::LF) {
			log.error("looking for \\n");
			pos = buffer.find(http::LF);
		} else {
			return false;
		}
		log.error("pos: " + i2a(pos));
		if (pos == -1) return false;

		size_t size = 0;

		for (ssize_t i = 0; i < pos; ++i) {

			char c = buffer.data[buffer.begin + i];
			if (c == ';') break;
			if (!isHexDigit(c)) {
				throw std::runtime_error("invalid chunk size");
			}

			int digit = hexDigitValue(c);
			size = size * 16 + digit;
			log.error("current size: " + i2a(size));

		}

		p.bytes_read_count = pos + p.line_end_size;
		p.chunk_size = size;
		log.error("chunk size: " + i2a(size));
		p.chunk_read = 0;

		if (size == 0)
			p.chunk_state = HTTPRequest::READING_TRAILERS;
		else
			p.chunk_state = HTTPRequest::READING_DATA;

		return true;

	}

	case HTTPRequest::READING_DATA: {

		log.error("READING DATA");
		size_t remaining = p.chunk_size - p.chunk_read;
		size_t available = buffer.end - buffer.mark;
		size_t count = std::min(remaining, available);
		log.error("count: " + i2a(count));

		if (count == 0) return false;

		/*
		* Temporarily make the buffer look like a buffer
		* containing only the current chunk's payload.
		*/
		size_t old_begin = buffer.begin;
		log.error("old_begin: " + i2a(old_begin));
		size_t old_mark = buffer.mark;
		log.error("old_mark: " + i2a(old_mark));
		size_t old_end = buffer.end;
		log.error("old_end: " + i2a(old_end));

		buffer.begin = buffer.mark;
		log.error("begin: " + i2a(buffer.begin));
		buffer.end = buffer.begin + count;
		log.error("end: " + i2a(buffer.end));
		buffer.mark = buffer.begin;
		log.error("mark: " + i2a(buffer.mark));

		log.debug(buffer.str());
		bool complete = _parseBody(buffer, request);

		size_t consumed = p.bytes_read_count;
		log.error("consumed: " + i2a(consumed));

		/*
		* Restore the real raw-buffer boundaries.
		*/
		buffer.end = old_end;
		buffer.mark = old_mark;
		buffer.begin = old_begin;

		if (consumed == 0) return complete;

		p.chunk_read += consumed;
		if (p.chunk_read == p.chunk_size) {
			p.chunk_state = HTTPRequest::LINE_BREAK;
			log.error("SET TO LINE_BREAK");
		}

		return complete;

	}

	case HTTPRequest::LINE_BREAK: {

		log.error("LINE BREAK");
		if (buffer.end - buffer.mark < p.line_end_size) return false;

		if (p.line_ending == HTTPRequest::CRLF) {
			if (buffer.data[buffer.mark] != '\r' ||
				buffer.data[buffer.mark + 1] != '\n')
				throw std::runtime_error("invalid chunk CRLF");
		} else if (p.line_ending ==  HTTPRequest::LF) {
			if (buffer.data[buffer.mark] != '\n')
				throw std::runtime_error("invalid chunk LF");
		} else {
			throw std::runtime_error("invalid chunk");
		}

		p.bytes_read_count = p.line_end_size;
		p.chunk_state = HTTPRequest::READING_SIZE;
		return true;

	}

	case HTTPRequest::READING_TRAILERS: {

		log.error("READING TRAILERS");
		return _parseHeaders(buffer, request);
		/*
		* No trailers: the terminating chunk is followed
		* immediately by the empty line.
		*
		* If you support actual trailer fields, parse them
		* here instead.
		*/
		// ssize_t pos = buffer.find("\r\n");

		// if (pos == std::string::npos) return false;

		// p.bytes_read_count = pos + 2;
		// p.chunk_state = HTTPRequest::COMPLETE;
		// return true;

	}

	case HTTPRequest::END_OF_CHUNKS:
		log.error("END OF CHUNKS");
		promoteFile(request);
		p.state = HTTPRequest::COMPLETE;
		return true;
	}

	return false;

}

bool Parser::_parseBody(const Buffer& buffer, HTTPRequest& request) {

	HTTPRequest::ParsingContext& p = request.parsing;
	p.bytes_read_count = 0;

	if (request.is_multipart) {

		ssize_t boundary_pos;
		// size_t boundary_end_pos;
		const std::string& boundary = request.body.boundary;

		switch (p.multipart_state) {

		case HTTPRequest::READING_PREAMBLE:

			log.error("READING PREAMBLE");
			boundary_pos = buffer.find(boundary);
			log.error("boundary: [" + boundary + "]");
			log.error("boundary_pos: " + i2a(boundary_pos));
			log.error("body bytes: " + i2a(buffer.range()));
			if (boundary_pos == -1) {

				/*
				* The boundary isn't present.
				*
				* Keep enough bytes at the end because a boundary
				* may start there and continue in the next buffer.
				*/
				size_t keep = boundary.size() - 1;
				size_t size = buffer.range();

				if (size <= keep) return false;

				size_t n = size - keep;
				p.bytes_read_count = n;
				return true;

			// } else if (boundary_pos == 0) {
   //
			// 	/*
			// 	* Boundary starts at buffer.begin.
			// 	* Nothing has been consumed yet.
			// 	*/
			// 	p.bytes_read_count = boundary_pos + boundary.size();
			// 	request.body.parts.push_back(HTTPRequest::BodyPart());
			// 	p.multipart_state = HTTPRequest::READING_PART_HEADERS;
			// 	return true;

			} else {

				/*
				* Everything before the boundary is garbage and should be discarded.
				*/
				p.bytes_read_count = boundary_pos + boundary.size();
				request.body.parts.push_back(HTTPRequest::BodyPart());
				p.multipart_state = HTTPRequest::READING_BOUNDARY;
				return true;

			}

		case HTTPRequest::READING_PART_HEADERS:

			return _parseHeaders(buffer, request);

		case HTTPRequest::READING_PART_BODY:

			// const std::string& boundary = request.body.boundary;
			boundary_pos = buffer.find(boundary);
			log.error("boundary: [" + boundary + "]");
			log.error("boundary_pos: " + i2a(boundary_pos));
			log.error("body bytes: " + i2a(buffer.range()));

			if (boundary_pos == -1) {

				/*
				* The boundary isn't present.
				*
				* Keep enough bytes at the end because a boundary
				* may start there and continue in the next buffer.
				*/
				size_t keep = boundary.size() - 1;
				size_t size = buffer.range();
				if (size <= keep) return false;

				size_t n = size - keep;
				ssize_t bytes_written = write(request.body.parts.back().file,
											  &buffer.data[buffer.begin], n);
				if (bytes_written < 0) {
					throw std::runtime_error("write: " + std::string(strerror(errno)));
				}

				// log.error("bytes written to file: " + buffer.substr(0, bytes_written));
				p.bytes_read_count = bytes_written;
				return true;

			} else if (boundary_pos == 0) {

				if (buffer.range() < boundary.size()) {
					return false;
				}

				p.bytes_read_count = boundary.size();
				p.multipart_state = HTTPRequest::READING_BOUNDARY;
				log.error("STATE SET TO READING_BOUNDARY");
				return true;

				/*
				* Boundary starts at buffer.begin.
				* Nothing has been consumed yet.
				*/
				// boundary_end_pos = buffer.begin + boundary.size();
				// if (buffer.data[boundary_end_pos] == '-' &&
				// buffer.data[boundary_end_pos + 1] == '-') {
    //
				// 	p.bytes_read_count += boundary.size() + 2;
				// 	p.multipart_state = HTTPRequest::END_OF_PART;
    //
				// 	if (p.line_ending == HTTPRequest::CRLF) {
				// 		if (buffer.data[boundary_end_pos + 2] == '\r' ||
				// 			buffer.data[boundary_end_pos + 3] == '\n') {
				// 			p.bytes_read_count += p.line_end_size;
				// 			log.error("STUPID TWO");
				// 		}
				// 	} else {
				// 		if (buffer.data[boundary_end_pos + 2] == '\n') {
				// 			p.bytes_read_count += p.line_end_size;
				// 			log.error("STUPID ONE");
				// 		}
				// 	}
    //
				// 	promoteFile(request);
				// 	log.error("Oh yeah!");
				// 	return true;
    //
				// } else {
    //
				// 	if (p.line_ending == HTTPRequest::CRLF) {
				// 		if (buffer.data[boundary_end_pos + 2] == '\r' ||
				// 			buffer.data[boundary_end_pos + 3] == '\n') {
				// 			p.bytes_read_count += p.line_end_size;
				// 			log.error("STUPID TWO");
				// 		} else {
				// 			p.multipart_state = HTTPRequest::FAILURE;
				// 			log.error("failure");
				// 			return false;
				// 		}
				// 	} else {
				// 		if (buffer.data[boundary_end_pos + 2] == '\n') {
				// 			p.bytes_read_count += p.line_end_size;
				// 			log.error("STUPID ONE");
				// 		} else {
				// 			p.multipart_state = HTTPRequest::FAILURE;
				// 			log.error("failure");
				// 			return false;
				// 		}
				// 	}
    //
				// 	p.multipart_state = HTTPRequest::READING_PART_HEADERS;
				// 	log.error("Somehow need more");
				// 	return true;

				// }

			} else {

				/*
				* Everything before the boundary is definitely part data.
				*/
				ssize_t bytes_written = write(request.body.parts.back().file,
											  &buffer.data[buffer.begin], boundary_pos);
				if (bytes_written < 0) {

					throw std::runtime_error("write: " + std::string(strerror(errno)));

				}

				if (bytes_written < boundary_pos) {

					// Don't advance past bytes that weren't written.
					p.bytes_read_count = bytes_written;
					return true;

				}

				// log.error("bytes written to file: " + buffer.substr(0, bytes_written));
				p.bytes_read_count = bytes_written + boundary.size();
				p.multipart_state = HTTPRequest::READING_BOUNDARY;
				log.error("STATE SET TO READING_BOUNDARY");
				return true;

			}

		case HTTPRequest::READING_BOUNDARY:

			log.error("HERE COMES THE BOUNDARY HUNTER!");
			/*
			* We already know the boundary starts at buffer.begin.
			* Nothing has been consumed yet.
			*/
			if (buffer.range() < 2) {
				log.error("Somehow need more");
				return false;
			}

			/*
			* After the boundary:
			*
			*   --boundary   -> next part
			*   --boundary-- -> final boundary
			*/
			// p.bytes_read_count = boundary.size();
			// boundary_end_pos = buffer.begin + boundary.size();

			if (buffer.data[buffer.begin] == '-' &&
				buffer.data[buffer.begin + 1] == '-') {

				/*
				* Final boundary:
				*
				*   --boundary--
				*/
				p.bytes_read_count += 2;
				p.multipart_state = HTTPRequest::END_OF_PART;

				promoteFile(request);
				// log.error("file promoted");

				if (p.line_ending == HTTPRequest::CRLF &&
					buffer.data[buffer.begin + 2] == '\r' &&
					buffer.data[buffer.begin + 3] == '\n') {

					p.bytes_read_count += p.line_end_size;
					log.error("consuming trailing CRLF");

				} else if (p.line_ending == HTTPRequest::LF &&
						   buffer.data[buffer.begin + 2] == '\n') {

					p.bytes_read_count += p.line_end_size;
					log.error("consuming trailing LF");

				} else {

					p.multipart_state = HTTPRequest::FAILURE;
					log.error("failure");
					return false;

				}

				return true;

			} else if (p.line_ending == HTTPRequest::CRLF &&
					  (buffer.data[buffer.begin] == '\r' &&
					   buffer.data[buffer.begin + 1] == '\n')) {

				/*
				* Normal boundary:
				*
				*   --boundary\r\n
				*/
				p.bytes_read_count += p.line_end_size;
				log.error("consuming trailing CRLF");

				p.multipart_state = HTTPRequest::READING_PART_HEADERS;

				// request.body.parts.push_back(HTTPRequest::BodyPart());

				return true;

			} else if (p.line_ending ==  HTTPRequest::LF &&
					   buffer.data[buffer.begin] == '\n') {

				/*
				* Normal boundary:
				*
				*   --boundary\n
				*/
				p.bytes_read_count += p.line_end_size;
				log.error("consuming trailing LF");

				p.multipart_state = HTTPRequest::READING_PART_HEADERS;

				// request.body.parts.push_back(HTTPRequest::BodyPart());

				return true;

			} else {

				p.multipart_state = HTTPRequest::FAILURE;
				log.error("failure");
				return false;

			}

			// // const std::string& boundary = request.body.boundary;
			// if (buffer.range() < boundary.size()) {
			// 	log.error("early fail");
			// 	return false;
			// }
   //
			// if (std::memcmp(&buffer.data[buffer.begin],
			// 				boundary.data(), boundary.size()) != 0) {
   //
			// 	p.multipart_state = HTTPRequest::FAILURE;
			// 	log.error("Something that is not a boundary appeared.");
			// 	return false;
   //
			// }
			/*
			* We have the complete boundary.
			*/
			// p.bytes_read_count = boundary.size();

			// if (buffer.range() < boundary.size() + 2) {
			// 	log.error("Too smol");
			// 	return false;
			// }

			/*
			* After the boundary:
			*
			*   --boundary\r\n   -> next part
			*   --boundary--\r\n -> final boundary
			*/
			// boundary_end_pos = buffer.begin + boundary.size();
   //
			// /*
			// * Final boundary:
			// *
			// *   --boundary--\r\n
			// */
			// if (buffer.data[boundary_end_pos] == '-' &&
			// 	buffer.data[boundary_end_pos + 1] == '-') {
   //
			// 	// p.bytes_read_count += 2;
			// 	if (buffer.range() < boundary.size() + 2 + p.line_end_size) {
			// 		log.error("STUPID ONE");
			// 		return false;
			// 	}
   //
			// 	if (p.line_ending == HTTPRequest::CRLF) {
			// 		if (buffer.data[boundary_end_pos + 2] != '\r' ||
			// 			buffer.data[boundary_end_pos + 3] != '\n') {
			// 			p.multipart_state = HTTPRequest::FAILURE;
			// 			log.error("STUPID TWO");
			// 			return false;
			// 		}
			// 	} else {
			// 		if (buffer.data[boundary_end_pos + 2] != '\n') {
			// 			p.multipart_state = HTTPRequest::FAILURE;
			// 			log.error("STUPID THREE");
			// 			return false;
			// 		}
			// 	}
   //
			// 	p.bytes_read_count = boundary.size() + 2 + p.line_end_size; // Consume "--\r\n".
			// 	p.multipart_state = HTTPRequest::END_OF_PART;
			// 	promoteFile(request);
			// 	log.error("Oh yeah!");
			// 	return true;
   //
			// }
   //
			// /*
			// * Normal boundary:
			// *
			// *   --boundary\r\n
			// */
			// if (buffer.data[boundary_end_pos] == '\r' &&
			// 	buffer.data[boundary_end_pos + 1] == '\n') {
   //
			// 	p.bytes_read_count = boundary.size() + 2; // Consume "\r\n".
			// 	p.multipart_state = HTTPRequest::READING_PART_HEADERS;
			// 	log.error("Somehow need more");
			// 	return true;
   //
			// }
   //
			// p.multipart_state = HTTPRequest::FAILURE;
			// log.error("failure");
			// return false;

		case HTTPRequest::END_OF_PART:
			return true;

		case HTTPRequest::FAILURE:
			return false;

		}

	} else {


		/*
		* Single Part Body:
		*/
		size_t n;
		if (request.body_chunked) {
			n = buffer.range();
		} else {
			size_t remaining = request.body.size - p.bytes_written_count;
			size_t available = buffer.range();
			n = std::min(remaining, available);
		}

		if (n == 0) return true;

		log.error("request body file: " + i2a(request.body.file));
		ssize_t bytes_written = write(request.body.file,
									  &buffer.data[buffer.begin], n);
		if (bytes_written < 0) {
			throw std::runtime_error("write: " + std::string(strerror(errno)));
		}

		p.bytes_read_count = bytes_written;
		p.bytes_written_count += bytes_written;
		if (!request.body_chunked && p.bytes_written_count == request.body.size) {
			// request.parsing.state = HTTPRequest::COMPLETE;
			promoteFile(request);
		}

		return true;

	}

	return false;

}

// bool Parser::_parseBody(const Buffer& buffer, HTTPRequest& request) {
//
// 	// Calculate body start position based on line ending style ("\n\n" or "\r\n\r\n")
// 	// request.parsing.blank_line_size = request.parsing.line_ending == CRLF ? CRLFCRLF_SIZE : LFLF_SIZE;
// 	// request.parsing.body_start_pos = request.parsing.headers_end_pos + request.parsing.blank_line_size;
// 	// request.parsing.request_size = request.parsing.body_start_pos + request.parsing.content_length;
// 	// DEBUG BEGIN
// 	// log.debug("request_line_end_pos: " + i2a(request.parsing.request_line_end_pos));
// 	// log.debug("line_end_size: " + i2a(request.parsing.line_end_size));
// 	// log.debug("headers_start_pos: " + i2a(request.parsing.headers_start_pos));
// 	// log.debug("headers_end_pos: " + i2a(request.parsing.headers_end_pos));
// 	// log.debug("headers_size: " + i2a(request.parsing.headers_size));
// 	// log.debug("blank_line_size: " + i2a(request.parsing.blank_line_size));
// 	// log.debug("body_start_pos: " + i2a(request.parsing.body_start_pos));
// 	// log.debug("content_length: " + i2a(request.parsing.content_length));
// 	// log.debug("request_size: " + i2a(request.parsing.request_size));
// 	// log.debug("buffer.size() + raw.size(): " + i2a(request.parsing.buffer.size() + raw.size()));
// 	// DEBUG END
// /* // TEST
// 	// Add raw bytes to buffer
// 	request.parsing.buffer.append(raw);
// 	// DEBUG BEGIN
// 	// log.debug("buffer.size(): " + i2a(buffer.size()));
// 	// DEBUG END
//
// 	// Check if complete body received
// 	if (request.parsing.buffer.size() < request.parsing.request_size) {
//
// 		request.parsing.bytes_read_count = request.parsing.buffer.size() - request.parsing.old_buffer_fill_level;
//
// 		// log.debug("Bytes read: " + i2a(bytes_read_count));
// 		// log.debug("Previous buffer fill level: " + i2a(old_buffer_fill_level));
//
// 		request.parsing.old_buffer_fill_level = request.parsing.buffer.size();
//
// 		// log.debug("Current buffer fill level: " + i2a(buffer.size()));
// 		// log.debug("Current data in buffer (request):\n");
// 		// log.notice((buffer));
//
// 		request.parsing.state = HTTPRequest::READING_BODY;
// 		return request.parsing.state;  // not enough data for body: wait for more
//
// 	} else {
//
// 		request.setBody(request.parsing.buffer.substr(request.parsing.body_start_pos, request.parsing.content_length));
//
// 		request.parsing.bytes_read_count = request.parsing.request_size - request.parsing.old_buffer_fill_level;
//
// 		// log.debug("Expected overflow: " + i2a(request.parsing.buffer.size() - request.parsing.request_size));
// 		// log.debug("Actual overfloow: " + i2a((request.parsing.buffer.size() - request.parsing.old_buffer_fill_level) - request.parsing.bytes_read_count));
//
// 		request.parsing.buffer.clear();
// 		request.parsing.state = HTTPRequest::COMPLETE;
// 		return request.parsing.state;
//
// 	}
// */ // TEST
//
// 	if (request.parsing.full_body_size == 0) {
// 		request.parsing.full_body_size = static_cast<std::streamoff>(request.body.size);
// 	}
//
// 	// request.parsing.old_stream_pos = static_cast<std::streamoff>(request.body.temp.tellp());
// 	// if (request.parsing.old_stream_pos < 0) return request.parsing.state;
//
// 	log.error("old_stream_pos: " + i2a(request.parsing.old_stream_pos) + "\trange: " + i2a(buffer.range()) + "\tfull_body_size: " + i2a(request.parsing.full_body_size));
// 	// if (request.parsing.old_stream_pos + static_cast<std::streamoff>(raw.size()) <= request.parsing.full_body_size) {
// 	if (request.parsing.old_stream_pos + static_cast<std::streamoff>(buffer.range()) < request.parsing.full_body_size) {
// 		log.error("not all");
// 		// request.body.temp << buffer.str();
// 		buffer.sstream(request.body.temp);
// 		if (!request.body.temp) {
// 			request.parsing.state = HTTPRequest::ERROR;
// 			request.parsing.error_cause = INTERNAL_SERVER_ERROR;
// 			return false;
// 		}
// 		std::streamoff stream_pos = static_cast<std::streamoff>(request.body.temp.tellp());
// 		if (stream_pos < 0) {
// 			request.parsing.state = HTTPRequest::ERROR;
// 			request.parsing.error_cause = INTERNAL_SERVER_ERROR;
// 			return false;
// 		}
// 		request.parsing.bytes_read_count = stream_pos - request.parsing.old_stream_pos;
// 		log.debug("bytes read: " + i2a(request.parsing.bytes_read_count));
// 		log.debug("previous buffer fill level: " + i2a(request.parsing.old_stream_pos));
// 		request.parsing.old_stream_pos = stream_pos;
// 		log.debug("current buffer fill level: " + i2a(stream_pos));
// 		log.debug("current data in buffer (request):\n");
// 		// log.notice(request.body.temp.str());
// 		request.parsing.state = HTTPRequest::READING_BODY;
// 		return true;  // not enough data for body: wait for more
// 	} else {
// 		log.error("FULL BODY PARSED");
// 		// size_t remaining = request.parsing.content_length - request.body.temp.str().size();
// 		size_t remaining = request.parsing.full_body_size - request.parsing.old_stream_pos;
// 		buffer.sstream(request.body.temp, 0, remaining);
// 		request.parsing.bytes_read_count = remaining;
// 		log.debug("bytes read: " + i2a(request.parsing.bytes_read_count));
// 		log.debug("previous buffer fill level: " + i2a(request.parsing.old_stream_pos));
// 		log.debug("current buffer fill level: " + i2a(request.parsing.full_body_size));
// 		log.debug("current data in buffer (request):");
// 		// log.notice(request.body.temp.str());
// 		// request.setBody(request.body.temp.str()); // TEST
//
// 		// request.body.temp.clear();
// 		// request.parsing.buffer.clear();
// 		request.parsing.state = HTTPRequest::COMPLETE;
// 		return true;
// 	}
//
// }

// request.body.temp.write(raw.c_str(), sizeof(raw));
// 	size_t old_temp_size = request.body.temp.tellp();
// 	request.body.temp << raw;
// 	size_t temp_size = request.body.temp.tellp();
//
// 	// Check if complete body received
// 	if (request.body.temp.tellp() <= request.parsing.content_length) {
// 		request.parsing.bytes_read_count = temp_size - old_temp_size;
// 		request.parsing.state = HTTPRequest::READING_BODY;
// 		return request.parsing.state;
// 	} else {
//
// 	}
