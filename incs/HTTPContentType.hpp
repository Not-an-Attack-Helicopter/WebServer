/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPContentType.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 22:12:48 by bstorck           #+#    #+#             */
/*   Updated: 2026/08/24 22:12:49 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_CONTENT_TYPE_HPP
#define HTTP_CONTENT_TYPE_HPP

#include "HTTPParameters.hpp"
#include <string>

namespace HTTPContentType {

	/*
	* ================================================================
	* extractContentType()
	* ================================================================
	*
	* Parses Content-Type and returns:
	*
	*     contentType = "text/plain"
	*
	* and optionally:
	*
	*     boundary = "----abc"
	*
	* If the media type is multipart/form-data, we additionally apply
	* RFC 7578's media-type registration:
	*
	*     boundary is REQUIRED
	*     no other optional parameters are defined
	*
	* Thus:
	*
	*     multipart/form-data; boundary=abc
	*
	* is valid.
	*
	*     multipart/form-data
	*
	* is invalid.
	*
	*     multipart/form-data; boundary=abc; foo=bar
	*
	* is rejected here as strict multipart/form-data validation.
	* ================================================================
	*/

	bool extractContentType(const std::string& headerValue,
							std::string& contentType,
							std::string* boundary,
							std::vector<HTTPParameters::MIMEParameter>* parameters);

}

#endif
