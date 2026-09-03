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
#include "../incs/templates.hpp"
#include "../incs/constexpr.hpp"
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"
#include <unistd.h>
#include <cstddef>
#include <cctype>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Instance	*/
Parser& Parser::instance(void) {
	static Parser instance;
	return instance;
}

// Feed raw bytes; returns the current parse_state:
bool Parser::buffer(Buffer& buffer, HTTPRequest& request) {

	switch (request.parsing.state) {

	case HTTPRequest::READING_REQUEST_LINE:
		return _parseRequestLine(buffer, request);
	case HTTPRequest::READING_HEADERS:
		return _parseHeaders(buffer, request);
	case HTTPRequest::READING_BODY:
		if (request.body_chunked) {
			try {
				return _parseChunks(buffer, request);
			} catch (std::exception& e) {
				log.error(e.what());
				return false;
			}
		} else {
			try {
				return _parseBody(buffer, request);
			} catch (std::exception& e) {
				log.error(e.what());
				return false;
			}
		}
	default:
		return false;

	}

}

Method Parser::matchMethod(const std::string& method) {

	static const std::string valid_methods[
		static_cast<int>(METHOD_COUNT)
	] = {
		"GET", "HEAD", "DELETE", "POST", "PUT"
	};
	for (std::size_t i = 0; i < static_cast<int>(METHOD_COUNT); ++i) {
		if (valid_methods[i] == method) {
			return static_cast<Method>(i);
		}
	}
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
	}
	return *this;
};

/*	@brief Destructor	*/
Parser::~Parser() {
	log.debug("Parser Destructor called");
	return;
};

ssize_t Parser::_findRequestLineEnd(const Buffer& buffer, HTTPRequest& request) {

	ssize_t LF_pos = buffer.find(http::LF);
	if (LF_pos == -1) return std::string::npos;
	if (LF_pos != 0 && buffer.data[LF_pos - 1] == http::CR) {
		request.parsing.line_ending = HTTPRequest::CRLF;
		return LF_pos - 1;
	}
	return LF_pos;

}

bool Parser::_extractTokens(const Buffer& buffer, HTTPRequest& request) {

	// Transform to stream
	std::stringstream ss;
	buffer.sstream(ss, 0, request.parsing.line_end_pos);
	if (ss.fail()) {
		log.error("parse error: failed to set stringstream content");
		request.parsing.error_cause = INTERNAL_SERVER_ERROR;
		return false;
	}

	// Validate number of tokens
	std::string method, target, version, extra;
	if (!(ss >> method >> target >> version)) {
		log.warn("request line: not enoug tokens found");
		request.parsing.error_cause = BAD_REQUEST;
		return false;
	}
	if (ss >> extra) {
		log.warn("request line: too many tokens found");
		request.parsing.error_cause = BAD_REQUEST;
		return false;
	}

	// Validate method
	const Method matched_method = matchMethod(method);
	if (matched_method >= METHOD_COUNT) {
		log.warn("request line: unknown method");
		request.parsing.error_cause = BAD_REQUEST;
	}
	if (matched_method == HEAD) {
		request.headers_only = true;
	}
	request.setMethod(matched_method);

	// Validate target
	if (target.empty() || target[0] != '/') {
		log.warn("request line: invalid target");
		request.parsing.error_cause = BAD_REQUEST;
		return false;
	}

	// Split target into path and query
	std::size_t query_start_pos = target.find('?');
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
		log.warn("request line: http version not supported");
		request.parsing.error_cause = HTTP_VERSION_NOT_SUPPORTED;
		return false;
	}
	request.setVersion(version);

	return true;

}

bool Parser::_parseHeaderLine(const Buffer& buffer, HTTPRequest& request) {

	// Find separator ':'
	std::size_t colon_pos = static_cast<std::size_t>(buffer.find(':'));
	if (colon_pos == 0) {
		log.warn("request: header field has no field-name");
		return false;
	}
	if (colon_pos == request.parsing.line_end_pos || colon_pos == std::string::npos) {
		log.warn("request: header field has no colon");
		return false;
	}
	if (request.parsing.state == HTTPRequest::READING_HEADERS) {
		for (std::size_t i = 0; i < colon_pos; ++i) {
			if (!isTChar(buffer.data[buffer.begin + i])) {
				log.warn("request header: invalid character or whitespace in field-name");
				log.warn("culprit: {" + i2a((int)(unsigned char)buffer.data[buffer.begin + i]) + "}");
				return false;
			}
		}
	}

	std::string key = buffer.substr(0, colon_pos);
	for (std::string::iterator it = key.begin(); it != key.end(); ++it) {
		*it = std::tolower(static_cast<unsigned char>(*it));
	}

	std::string value = buffer.substr(colon_pos + 1, request.parsing.line_end_pos);

	// Trim whitespaces from value
	value = trim(value);

	for (std::size_t i = 0; i < value.size(); ++i) {
		unsigned char c = static_cast<unsigned char>(value[i]);
		if ((c < 0x20 && c != '\t') || c == 0x7f) {
			log.warn("request header: corrupt data in field-value");
			log.warn("culprit: {" + i2a((int)(unsigned char)value[i]) + "}");
			return false;
		}
	}

	if (request.parsing.state == HTTPRequest::READING_HEADERS) {
		request.setHeader(key, value);
	} else if (request.parsing.state == HTTPRequest::READING_BODY) {
		if (request.parsing.chunk_state ==  HTTPRequest::READING_TRAILERS) {
			request.body.setHeader(key, value);
		} else {
			request.body.parts.back().setHeader(key, value);
		}
	}

	return true;

}

bool Parser::_parseRequestLine(const Buffer& buffer, HTTPRequest& request) {

	// Find where request line ends
	request.parsing.line_end_pos = _findRequestLineEnd(buffer, request);

	// Calculate headers start position based on line ending style ("\n" or "\r\n")
	if (request.parsing.line_ending == HTTPRequest::CRLF) {
		request.parsing.line_end_size = CRLF_SIZE;
		request.parsing.blank_line_size = CRLFCRLF_SIZE;
	} else {
		request.parsing.line_end_size = LF_SIZE;
		request.parsing.blank_line_size = CRLF_SIZE;
	}

	// Detected empty line (before start of request line): not copied into buffer
	if (request.parsing.line_end_pos == 0) {

		// Set byte count to line_end_size to drop from data
		request.parsing.bytes_read_count = request.parsing.line_end_size;
		return true;

	// No line feed detected (Data only): wait for more data
	} else if (request.parsing.line_end_pos == std::string::npos) {

		request.parsing.bytes_read_count = std::string::npos;
		return false;

	// Maximum request line length exceeded
	} else if (request.parsing.line_end_pos > HTTPRequest::MAX_REQUEST_LINE_LENGTH) {
		log.warn("parse error: maximum request line length exceeded");
		request.parsing.error_cause = URI_TOO_LONG;
		request.parsing.state = HTTPRequest::ERROR;
		return false;

	// Line feed detected (end of request line): procced with line parsing
	} else {

		request.parsing.bytes_read_count =	request.parsing.line_end_pos +
											request.parsing.line_end_size;

		if (!_extractTokens(buffer, request)) {
			request.parsing.state = HTTPRequest::ERROR;
			return false;
		}

		request.parsing.state = HTTPRequest::READING_HEADERS;
		return true;
	}

}

bool Parser::_parseHeaders(const Buffer& buffer, HTTPRequest& request) {

	// Check for line break
	if (request.parsing.line_ending == HTTPRequest::CRLF) {
		request.parsing.line_end_pos = buffer.find(http::CRLF);
	} else {
		request.parsing.line_end_pos = buffer.find(http::LF);
	}

	// Empty line detected, proceed with validity checks
	if (request.parsing.line_end_pos == 0) {

		request.parsing.bytes_read_count = request.parsing.line_end_size;

		if (request.parsing.state == HTTPRequest::READING_HEADERS) {

			const std::string* content_disposition = request.getHeader("content-disposition");
			if (content_disposition !=  NULL) {
				if (!extractContentDisposition(*content_disposition,
											   HTTPContentDisposition::HTTP,
											   request.body.disposition,
											   &request.body.name,
											   &request.body.filename,
											   &request.body.filenameStar,
											   &request.body.d_parameters)) {
					log.warn("request: invalid content-disposition header provided");
				}
			}

			const std::string* type = request.getHeader("content-type");
			if (type != NULL) {
				if (!HTTPContentType::extractContentType(*type, request.body.type,
														 &request.body.boundary,
														 &request.body.t_parameters)) {
					log.warn("request: invalid content-type header provided");
				}
				if (equalCI(request.body.type, "multipart/form-data")) {
					request.is_multipart = true;
				}
			}

			const std::string* transfer_endcoding = request.getHeader("transfer-encoding");
			if (transfer_endcoding != NULL && equalCI(*transfer_endcoding, "chunked")) {
				request.body_chunked = true;
			}

			// Check for Host Header (mandatory for HTTP/1.1)
			if (request.getVersion() == http::V_1_1 && request.getHeader("host") == NULL) {
				log.warn("request: no host header provided");
				request.parsing.state = HTTPRequest::ERROR;
				request.parsing.error_cause = BAD_REQUEST;
				return false;
			}

			// GET and HEAD are not designed to carry a request body
			const Method requested_method = request.getMethod();
			if (requested_method == GET || requested_method == HEAD) {
				request.parsing.state = HTTPRequest::RESOLVING_ROUTE;
				return true;
			}

			// DELETE may come with a body (optional)
			if (requested_method == DELETE) {
				if (request.getHeader("content_length") !=  NULL) {
					if (!request.extractContentLength()) {
						log.info("request: no content-length header provided");
						request.body.size = 0;
					}
				} else {
					request.parsing.state = HTTPRequest::RESOLVING_ROUTE;
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

			//  Check if Content-Length value exceeds global body size treshold
			if (request.body.size > Config::SERVER_MAX_BODY_SIZE) {
				log.warn("request: content-length exceeds global treshold");
				request.parsing.error_cause = PAYLOAD_TOO_LARGE;
				request.parsing.state = HTTPRequest::ERROR;
				return false;
			}

			request.parsing.state = HTTPRequest::RESOLVING_ROUTE;

		} else if (request.parsing.state == HTTPRequest::READING_BODY) {

			if (request.parsing.chunk_state == HTTPRequest::READING_TRAILERS) {

				request.parsing.chunk_state = HTTPRequest::END_OF_CHUNKS;

			} else {

				const std::string* content_disposition = request.body.parts.back().getHeader("content-disposition");
				if (content_disposition != NULL) {
					if (!extractContentDisposition(*content_disposition,
												HTTPContentDisposition::MULTIPART_FORM_DATA,
												request.body.parts.back().disposition,
												&request.body.parts.back().name,
												&request.body.parts.back().filename)) {
						log.warn("request: invalid content-disposition part header provided");
					}
				}

				const std::string* type = request.getHeader("content-type");
				if (type != NULL) {
					if (!HTTPContentType::extractContentType(*type, request.body.parts.back().type,
															NULL, &request.body.parts.back().t_parameters)) {
						log.warn("request: invalid content-type part header provided");
					}
				}

				request.parsing.multipart_state = HTTPRequest::READING_PART_BODY;
				try {
					createFile(request);
				} catch (std::exception& e) {
					request.parsing.error_cause = INTERNAL_SERVER_ERROR;
					request.parsing.state = HTTPRequest::ERROR;
					log.warn(e.what());
					return false;
				}

			}

		}

		return true;

	// No line break detected, wait for more data
	} else if (request.parsing.line_end_pos == std::string::npos) {

		log.debug("waiting for more data...");
		request.parsing.bytes_read_count = std::string::npos;
		return false;

	// Maximum header line length exceeded
	} else if (request.parsing.line_end_pos > HTTPRequest::MAX_HEADER_LINE_LENGTH) {
		log.warn("request: maximum header line length exceeded");
		request.parsing.error_cause = REQUEST_HEADER_FIELDS_TOO_LARGE;
		request.parsing.state = HTTPRequest::ERROR;
		return false;

	// Detected line break, parse line
	} else {

		request.parsing.headers_size += request.parsing.line_end_pos;
		if (request.parsing.headers_size > HTTPRequest::MAX_TOTAL_HEADERS_SIZE) {
			log.warn("request: total header size exceeds the maximum allowed");
			request.parsing.error_cause = REQUEST_HEADER_FIELDS_TOO_LARGE;
			request.parsing.state = HTTPRequest::ERROR;
			return false;
		}

		if (!_parseHeaderLine(buffer, request)) {
			log.error("parse error: something went wrong while parsing a header line");
			request.parsing.state = HTTPRequest::ERROR;
			request.parsing.error_cause = BAD_REQUEST;
			return false;
		}

		request.parsing.bytes_read_count = request.parsing.line_end_pos + request.parsing.line_end_size;
		return true;

	}

}

static bool spoolBody(std::string body, int fd) {

	const char *data = body.c_str();
	ssize_t bytes_left = static_cast<ssize_t>(body.size());

	while (bytes_left > 0) {
		ssize_t bytes_written = write(fd, data, bytes_left);

		if (bytes_written > 0) {
			data += bytes_written;
			bytes_left -= bytes_written;
		}
		else if (bytes_written == -1 && errno == EINTR) {
			continue;
		}
		else {
			return false;
		}
	}

    return true;
}

bool Parser::_parseChunks(Buffer& buffer, HTTPRequest& request) {

	HTTPRequest::ParsingContext& p = request.parsing;
	p.bytes_read_count = 0;

	switch (p.chunk_state) {

	case HTTPRequest::READING_SIZE: {

		ssize_t pos;
		if (p.line_ending == HTTPRequest::CRLF) {
			pos = buffer.find(http::CRLF);
		} else if (p.line_ending == HTTPRequest::LF) {
			pos = buffer.find(http::LF);
		} else {
			return false;
		}

		if (pos == -1) return false;

		std::size_t size = 0;

		for (ssize_t i = 0; i < pos; ++i) {

			char c = buffer.data[buffer.begin + i];
			if (c == ';') break;
			if (!isHexDigit(c)) {
				throw std::runtime_error("invalid chunk size");
			}

			int digit = hexDigitValue(c);
			size = size * 16 + digit;

		}

		p.bytes_read_count = pos + p.line_end_size;
		p.chunk_size = size;
		p.chunk_read = 0;

		if (size == 0)
			p.chunk_state = HTTPRequest::READING_TRAILERS;
		else
			p.chunk_state = HTTPRequest::READING_DATA;

		return true;

	}

	case HTTPRequest::READING_DATA: {

		std::size_t remaining = p.chunk_size - p.chunk_read;
		std::size_t available = buffer.end - buffer.mark;
		std::size_t count = std::min(remaining, available);

		if (count == 0) return false;

		/*
		* Temporarily make the buffer look like a buffer
		* containing only the current chunk's payload.
		*/
		std::size_t old_begin = buffer.begin;
		std::size_t old_mark = buffer.mark;
		std::size_t old_end = buffer.end;

		buffer.begin = buffer.mark;
		buffer.end = buffer.begin + count;
		buffer.mark = buffer.begin;

		bool complete = _parseBody(buffer, request);

		std::size_t consumed = p.bytes_read_count;

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
		}

		return complete;

	}

	case HTTPRequest::LINE_BREAK: {

		if (buffer.end - buffer.mark < p.line_end_size) {
			return false;
		}

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

		return _parseHeaders(buffer, request);
		/*
		* No trailers: the terminating chunk is followed
		* immediately by the empty line.
		*
		* If you support actual trailer fields, parse them
		* here instead.
		*/
	}

	case HTTPRequest::END_OF_CHUNKS:
		p.state = HTTPRequest::COMPLETE;
		return true;
	}

	return false;

}

bool Parser::_parseBody(const Buffer& buffer, HTTPRequest& request) {

	HTTPRequest::ParsingContext& p = request.parsing;
	p.bytes_read_count = 0;

	if (request.is_multipart && !request.requires_CGI) {

		/*
		* Multipart Body (NOT requiring CGI):
		*/
		ssize_t boundary_pos;
		const std::string& boundary = request.body.boundary;

		switch (p.multipart_state) {

		case HTTPRequest::PREAMBLE:

			boundary_pos = buffer.find(boundary);
			if (boundary_pos == -1) {

				/*
				* The boundary isn't present.
				*
				* Keep enough bytes at the end because a boundary
				* may start there and continue in the next buffer.
				*/
				std::size_t keep = boundary.size() - 1;
				std::size_t size = buffer.range();

				if (size <= keep) return false;

				std::size_t n = size - keep;
				p.bytes_read_count = n;
				return true;

			} else {

				/*
				* Everything before the boundary is garbage and should be discarded.
				*/
				request.body.parts.push_back(HTTPRequest::BodyPart());
				p.bytes_read_count = boundary_pos + boundary.size();
				p.multipart_state = HTTPRequest::BOUNDARY;
				return true;

			}

		case HTTPRequest::READING_PART_HEADERS:

			return _parseHeaders(buffer, request);

		case HTTPRequest::READING_PART_BODY:

			boundary_pos = buffer.find(boundary);

			if (boundary_pos == -1) {

				/*
				* The boundary isn't present.
				*
				* Keep enough bytes at the end because a boundary
				* may start there and continue in the next buffer.
				*/
				std::size_t keep = boundary.size() - 1;
				std::size_t size = buffer.range();
				if (size <= keep) return false;

				std::size_t n = size - keep;
				ssize_t bytes_written = write(request.body.parts.back().file,
											  &buffer.data[buffer.begin], n);
				if (bytes_written < 0) {
					throw std::runtime_error("write: " + std::string(strerror(errno)));
				}

				p.bytes_read_count = bytes_written;
				return true;

			} else if (boundary_pos == 0) {

				if (buffer.range() < boundary.size()) {
					return false;
				}

				p.bytes_read_count = boundary.size();
				p.multipart_state = HTTPRequest::BOUNDARY;
				return true;

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

				p.bytes_read_count = bytes_written + boundary.size();
				p.multipart_state = HTTPRequest::BOUNDARY;
				return true;

			}

		case HTTPRequest::BOUNDARY:

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

			if (buffer.data[buffer.begin] == '-' &&
				buffer.data[buffer.begin + 1] == '-') {

				/*
				* Final boundary:
				*
				*   --boundary--
				*/
				p.bytes_read_count += 2;
				p.multipart_state = HTTPRequest::END_OF_PARTS;

				if (p.line_ending == HTTPRequest::CRLF &&
					buffer.data[buffer.begin + 2] == '\r' &&
					buffer.data[buffer.begin + 3] == '\n') {

					p.bytes_read_count += p.line_end_size;

				} else if (p.line_ending == HTTPRequest::LF &&
						   buffer.data[buffer.begin + 2] == '\n') {

					p.bytes_read_count += p.line_end_size;

				} else {

					log.error("failure at end of parts");
					p.error_cause = INTERNAL_SERVER_ERROR;
					p.state = HTTPRequest::ERROR;
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

				p.multipart_state = HTTPRequest::READING_PART_HEADERS;
				return true;

			} else if (p.line_ending ==  HTTPRequest::LF &&
					   buffer.data[buffer.begin] == '\n') {

				/*
				* Normal boundary:
				*
				*   --boundary\n
				*/
				p.bytes_read_count += p.line_end_size;

				p.multipart_state = HTTPRequest::READING_PART_HEADERS;
				return true;

			} else {

				// p.multipart_state = HTTPRequest::FAILURE;
				log.error("failure at part end");
				p.error_cause = INTERNAL_SERVER_ERROR;
				p.state = HTTPRequest::ERROR;
				return false;

			}

		case HTTPRequest::END_OF_PARTS:
			p.state =  HTTPRequest::COMPLETE;
			return true;

		}

	} else if (request.requires_CGI) {

		/*
		* Body is input for CGI stdin:
		*/
		std::size_t n;
		if (request.body_chunked) {
			n = buffer.range();
		} else {
			std::size_t remaining = request.body.size - p.bytes_written_count;
			std::size_t available = buffer.range();
			n = std::min(remaining, available);
		}

		if (n == 0) return true;

		ssize_t bytes_consumed = write(request.cgi.std_in, &buffer.data[buffer.begin], n);
		if (bytes_consumed < 0) {
			throw std::runtime_error("write: " + std::string(strerror(errno)));
		}

		p.bytes_read_count = bytes_consumed;
		p.bytes_written_count += bytes_consumed;
		return true;

	} else {

		/*
		* Single Part Body:
		*/
		if (request.body.sink == NONE) {
			try {
				createFile(request);
			} catch (std::exception& e) {
				request.parsing.error_cause = INTERNAL_SERVER_ERROR;
				request.parsing.state = HTTPRequest::ERROR;
				log.warn(e.what());
				return false;
			}
		}

		std::size_t n;
		if (request.body_chunked) {
			n = buffer.range();
		} else {
			std::size_t remaining = request.body.size - p.bytes_written_count;
			std::size_t available = buffer.range();
			n = std::min(remaining, available);
		}

		if (n == 0) return true;

		ssize_t bytes_written = 0;
		switch (request.body.sink) {

		case HEAP:
			bytes_written = n;
			request.body.temp.append(buffer.str(), n);
			if (request.body.temp.size() > HTTPRequest::MAX_HEAP_STORED_BODY_SIZE) {
				try {
					createFile(request);
				} catch (std::exception& e) {
					request.parsing.error_cause = INTERNAL_SERVER_ERROR;
					request.parsing.state = HTTPRequest::ERROR;
					log.warn(e.what());
					return false;
				}
				if (!spoolBody(request.body.temp, request.body.file)) {
					request.parsing.error_cause = INTERNAL_SERVER_ERROR;
					request.parsing.state = HTTPRequest::ERROR;
					return false;
				}
			}
			break;

		case DISK:
			log.error("request body file: " + i2a(request.body.file));
			bytes_written = write(request.body.file, &buffer.data[buffer.begin], n);
			if (bytes_written < 0) {
				throw std::runtime_error("write: " + std::string(strerror(errno)));
			}
			break;

		case NONE:
			request.parsing.error_cause = INTERNAL_SERVER_ERROR;
			request.parsing.state = HTTPRequest::ERROR;
			return false;
		}

		p.bytes_read_count = bytes_written;
		p.bytes_written_count += bytes_written;
		return true;
	}
	return false;
}
