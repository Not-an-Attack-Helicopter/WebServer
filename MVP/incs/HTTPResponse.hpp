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

#include <string>
#include <map>

class HTTPResponse {

	public:

		HTTPResponse(void);
		~HTTPResponse(void);

		// Status
		unsigned int								getStatusCode(void) const;
		const std::string&							getStatusReason(void) const;
		void										setStatus(int code);
		void										setStatus(int code,
															  const std::string& reason);

		// Body, Content-Type, and Content-Length
		const std::string&							getBody(void) const;
		void										setBody(const std::string& body,
															const std::string& content_type);
		// Headers
		// const std::map<std::string, std::string>&	getHeaders(void) const;
		void										setHeader(const std::string& key,
															const std::string& value);

		// Produce the raw HTTP/1.1 string ready to write to the socket
		std::string									serialize(void) const;
		void										reset(void);

	private:

		HTTPResponse(const HTTPResponse& other);
		HTTPResponse& operator = (const HTTPResponse& other);

		static std::string							_getDefaultReason(int code);

		int											_status_code;
		std::string									_status_reason;
		std::string									_body;
		std::map<std::string, std::string>			_headers;

};

#endif
