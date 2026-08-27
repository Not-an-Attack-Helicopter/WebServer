/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequestParser.hpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sholz, bstorck <marvin@42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 18:41:59 by sholz             #+#    #+#             */
/*   Updated: 2026/08/24 21:57:21 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include "HTTPRequest.hpp"
// #include "HTTPResponse.hpp"
#include "Config.hpp"
#include "Buffer.hpp"
// #include "utils.hpp"
// #include <stdexcept>
// #include <sstream>
// #include <vector>
#include <string>
// #include <map>
// #include <climits>
#include <cstddef>
// #include <typeinfo>
#include <sys/stat.h>

#define parse Parser::instance()

class Parser {

public:

	static Parser&							instance(void);

	// HTTPRequest::ParseState					incomingData(const std::string& raw, HTTPRequest* request);
	bool									buffer(Buffer& buff, HTTPRequest& request);
	// bool									body(const Client::Buffer& buff, HTTPRequest& request);

	Method									extractMethod(const std::string& name);

private:

	Parser(void);
	Parser(const Parser& other);
	Parser& operator = (const Parser& other);
	~Parser(void);

	static const size_t 					LF_SIZE = 1;
	static const size_t 					CRLF_SIZE = 2;
	static const size_t						LFLF_SIZE = 2;
	static const size_t						CRLFCRLF_SIZE = 4;

	size_t									_findRequestLineEnd(const Buffer& buffer, HTTPRequest& request);

	// bool									_matchMethod(const std::string& method);
	bool									_extractTokens(const Buffer& buffer, HTTPRequest& request);
	bool									_parseHeaderLine(const Buffer& buffer, HTTPRequest& request);
	// bool									_extractContentLength(void);

	bool									_parseRequestLine(const Buffer& buffer, HTTPRequest& request);
	bool									_parseHeaders(const Buffer& buffer, HTTPRequest& request);

	bool									_parseSize(const Buffer& input, Buffer& output);
	bool									_parseData(const Buffer& input, Buffer& output);
	bool									_parseDataCRLF(const Buffer& input, Buffer& output);
	bool									_parseTrailers(const Buffer& input, Buffer& output);

	bool									_parseChunks(Buffer& input, HTTPRequest& request);

	bool									_parseBody(const Buffer& buffer, HTTPRequest& request);

};

// #include "Parser.tpp"

#endif
