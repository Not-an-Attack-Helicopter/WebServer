/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPContentType.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 22:35:16 by bstorck           #+#    #+#             */
/*   Updated: 2026/08/24 22:35:17 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/HTTPContentType.hpp"
#include "../incs/HTTPGrammar.hpp"
#include "../incs/utils.hpp"

namespace HTTPContentType {

	/*
	* ================================================================
	* RFC 2046 multipart boundary validation
	* ================================================================
	*
	* multipart boundary:
	*
	*     1*69<bchars> bcharsnospace
	*
	* The practical restriction is:
	*
	*     1..70 octets
	*     final character is not SP
	*
	* bcharsnospace:
	*
	*     DIGIT / ALPHA / "'" / "(" / ")" / "+" / "_"
	*     / "," / "-" / "." / "/" / ":" / "=" / "?"
	*
	* bchars additionally allows SP.
	*
	* The value returned by parseContentType() has already had
	* quoted-string delimiters removed.
	* ================================================================
	*/

	static bool isBChar(char c) {

		unsigned char uc = static_cast<unsigned char>(c);
		if ((uc >= '0' && uc <= '9') ||
			(uc >= 'A' && uc <= 'Z') ||
			(uc >= 'a' && uc <= 'z')) {
			return true;
		}

		switch (uc) {

		case '\'':
		case '(':
		case ')':
		case '+':
		case '_':
		case ',':
		case '-':
		case '.':
		case '/':
		case ':':
		case '=':
		case '?':
		case ' ':
		return true;

		default:
		return false;

		}
	}

	// static bool isBoundaryCharNoSpace(char c) {
	//
	// 	if (c == ' ') return false;
	//
	// 	return isBoundaryChar(c);
	//
	// }

	static bool validMultipartBoundary(const std::string& boundary) {

		/*
		* RFC 2046: boundary length is 1..70.
		*/
		if (boundary.empty() || boundary.size() > 70) {
			return false;
		}

		/*
		* Every character MUST be a bchars.
		*/
		for (std::size_t i = 0; i < boundary.size(); ++i) {
			if (!isBChar(boundary[i])) {
				return false;
			}
		}

		/*
		* Last character MUST be bcharsnospace.
		*/
		// if (!isBoundaryCharNoSpace(boundary[boundary.size() - 1])) {
		// 	return false;
		// }
		// const char last = boundary[boundary.size() - 1];
		if (boundary[boundary.size() - 1] ==  ' ') {
			return false;
		}
		// if (!isBChar(last)) {
		// 	return false;
		// }

		return true;

	}

	/*
	* ================================================================
	* RFC 9110 Content-Type
	* ================================================================
	*
	* media-type = type "/" subtype parameters
	*
	* type       = token
	* subtype    = token
	* ================================================================
	*/

	static bool parseContentType(const std::string& headerValue,
								 HTTPParameters::MIMEValue& result) {

		result.value.clear();
		result.parameters.clear();

		std::size_t pos = 0;

		HTTPGrammar::skipOWS(headerValue, pos);

		/*
		* type
		*/
		std::string type;

		if (!HTTPGrammar::parseToken(headerValue, pos, type)) {
			return false;
		}

		/*
		* RFC 9110 requires '/' immediately after type.
		*/
		if (pos >= headerValue.size() || headerValue[pos] != '/') {
			return false;
		}

		++pos;

		/*
		* subtype
		*/
		std::string subtype;

		if (!HTTPGrammar::parseToken(headerValue, pos, subtype)) {
			return false;
		}

		/*
		* Type and subtype are case-insensitive.
		*/
		result.value = tolowerASCII(type) + "/" + tolowerASCII(subtype);

		/*
		* Parameters.
		*/
		if (!parseParameters(headerValue, pos, result.parameters, HTTPParameters::RFC9110)) {
			result.value.clear();
			result.parameters.clear();
			return false;
		}

		return true;
	}

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
							std::vector<HTTPParameters::MIMEParameter>* parameters) {

		HTTPParameters::MIMEValue parsed;

		contentType.clear();

		if (boundary != NULL) boundary->clear();

		if (parameters != NULL) parameters->clear();

		if (!parseContentType(headerValue, parsed)) {
			return false;
		}

		contentType = parsed.value;

		if (parameters != NULL) *parameters = parsed.parameters;

		/*
		* Normal non-multipart Content-Type.
		*/
		if (!equalCI(parsed.value, "multipart/form-data")) {
			if (boundary != NULL) {
				boundary->clear();
			}
			return true;
		}

		/*
		* multipart/form-data:
		*
		* boundary is REQUIRED.
		*/
		std::string boundaryValue;
		std::size_t boundaryCount = 0;

		for (std::size_t i = 0; i < parsed.parameters.size(); ++i) {
			if (equalCI(parsed.parameters[i].name, "boundary")) {
				++boundaryCount;
				if (boundaryCount == 1) boundaryValue = parsed.parameters[i].value;
			}
		}

		if (boundaryCount != 1) return false;

		/*
		* RFC 7578 registration says no optional parameters.
		*/
		if (parsed.parameters.size() != 1) return false;

		/*
		* Validate the actual boundary syntax.
		*/
		if (!validMultipartBoundary(boundaryValue)) return false;

		if (boundary != NULL) *boundary = "--" + boundaryValue;

		return true;

	}

}
