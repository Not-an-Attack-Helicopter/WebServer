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
// #include "Config.hpp"
#include "Buffer.hpp"
// #include <string>
// #include <cstddef>
// #include <sys/stat.h>

#define parse RequestParser::instance()

class RequestParser {

public:

	static RequestParser&					instance(void);

	bool									buffer(Buffer& buff, HTTPRequest& request);

	Method									matchMethod(const std::string& name);

private:

	RequestParser(void);
	RequestParser(const RequestParser& other);
	RequestParser& operator = (const RequestParser& other);
	~RequestParser(void);

	static const std::size_t 				LF_SIZE = 1;
	static const std::size_t 				CRLF_SIZE = 2;
	static const std::size_t				LFLF_SIZE = 2;
	static const std::size_t				CRLFCRLF_SIZE = 4;

	ssize_t									_findRequestLineEnd(const Buffer& buffer, HTTPRequest& request);

	bool									_extractTokens(const Buffer& buffer, HTTPRequest& request);
	bool									_parseHeaderLine(const Buffer& buffer, HTTPRequest& request);

	bool									_parseRequestLine(const Buffer& buffer, HTTPRequest& request);
	bool									_parseHeaders(const Buffer& buffer, HTTPRequest& request);

	bool									_parseSize(const Buffer& input, Buffer& output);
	bool									_parseData(const Buffer& input, Buffer& output);
	bool									_parseDataCRLF(const Buffer& input, Buffer& output);
	bool									_parseTrailers(const Buffer& input, Buffer& output);

	bool									_parseChunks(Buffer& input, HTTPRequest& request);

	bool									_parseBody(const Buffer& buffer, HTTPRequest& request);

};

#endif
