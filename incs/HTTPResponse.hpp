/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/09 01:07:51 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/09 01:07:53 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "Config.hpp"
// #include <cstddef>
// #include <string>
// #include <map>

enum StatusCode {
	NO_STATUS = 0,
	// 1xx Informational
	CONTINUE = 100,
	SWITCHING_PROTOCOLS = 101,
	PROCESSING = 102,
	EARLY_HINTS = 103,
	// 2xx Success
	OK = 200,
	CREATED = 201,
	ACCEPTED = 202,
	NON_AUTHORITATIVE_INFORMATION = 203,
	NO_CONTENT = 204,
	RESET_CONTENT = 205,
	PARTIAL_CONTENT = 206,
	MULTI_STATUS = 207,
	ALREADY_REPORTED = 208,
	IM_USED = 226,
	// 3xx Redirection
	MULTIPLE_CHOICES = 300,
	MOVED_PERMANENTLY = 301,
	FOUND = 302,
	SEE_OTHER = 303,
	NOT_MODIFIED = 304,
	USE_PROXY = 305,
	TEMPORARY_REDIRECT = 307,
	PERMANENT_REDIRECT = 308,
	// 4xx Client Error
	BAD_REQUEST = 400,
	UNAUTHORIZED = 401,
	PAYMENT_REQUIRED = 402,
	FORBIDDEN = 403,
	NOT_FOUND = 404,
	METHOD_NOT_ALLOWED = 405,
	NOT_ACCEPTABLE = 406,
	PROXY_AUTHENTICATION_REQUIRED = 407,
	REQUEST_TIMEOUT = 408,
	CONFLICT = 409,
	GONE = 410,
	LENGTH_REQUIRED = 411,
	PRECONDITION_FAILED = 412,
	PAYLOAD_TOO_LARGE = 413,
	URI_TOO_LONG = 414,
	UNSUPPORTED_MEDIA_TYPE = 415,
	RANGE_NOT_SATISFIABLE = 416,
	EXPECTATION_FAILED = 417,
	IM_A_TEAPOT = 418,
	MISDIRECTED_REQUEST = 421,
	UNPROCESSABLE_ENTITY = 422,
	LOCKED = 423,
	FAILED_DEPENDENCY = 424,
	TOO_EARLY = 425,
	UPGRADE_REQUIRED = 426,
	PRECONDITION_REQUIRED = 428,
	TOO_MANY_REQUESTS = 429,
	REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
	UNAVAILABLE_FOR_LEGAL_REASONS = 451,
	// 5xx Server Error
	INTERNAL_SERVER_ERROR = 500,
	NOT_IMPLEMENTED = 501,
	BAD_GATEWAY = 502,
	SERVICE_UNAVAILABLE = 503,
	GATEWAY_TIMEOUT = 504,
	HTTP_VERSION_NOT_SUPPORTED = 505,
	VARIANT_ALSO_NEGOTIATES = 506,
	INSUFFICIENT_STORAGE = 507,
	LOOP_DETECTED = 508,
	NOT_EXTENDED = 510,
	NETWORK_AUTHENTICATION_REQUIRED = 511
};

class HTTPResponse {

public:

	HTTPResponse(void);
	~HTTPResponse(void);

	// Status and Status Reason
	unsigned int								getStatusCode(void) const;
	const std::string&							getStatusReason(void) const;
	void										setStatus(StatusCode code);
	void										setStatus(StatusCode code,
														  const std::string& reason);

	// Headers
	const std::map<std::string, std::string>&	getHeaders(void) const;

	void										setHeader(const std::string& key,
														  const std::string& value);

	// Body(-Type), Content-Type, and Content-Length
	void										setBody(const std::string& body,
														const std::string& content_type,
														bool headers_only);
	const std::string&							getBody(void) const;
	std::size_t									getBodySize(void) const;
	Sink										getBodySink(void) const;
	void										setBodySink(Sink body_type);
	void										reset(void);

private:

	HTTPResponse(const HTTPResponse& other);
	HTTPResponse& operator = (const HTTPResponse& other);

	static std::string							_getDefaultReason(StatusCode code);

	StatusCode									_status_code;

	std::string									_status_reason;

	std::map<std::string, std::string>			_headers;

	std::string									_body;

	std::size_t									_body_size;

	Sink										_body_sink;

};

#endif
