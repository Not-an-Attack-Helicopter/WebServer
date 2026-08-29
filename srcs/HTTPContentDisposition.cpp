/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPContentDisposition.cpp                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 22:35:26 by bstorck           #+#    #+#             */
/*   Updated: 2026/08/24 22:35:27 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/HTTPContentDisposition.hpp"
#include "../incs/HTTPGrammar.hpp"
#include "../incs/utils.hpp"

namespace HTTPContentDisposition {

	/*
	* ================================================================
	* RFC 5987 / RFC 6266 helpers
	* ================================================================
	*
	* HTTP Content-Disposition supports:
	*
	*     filename="example.txt"
	*
	* and:
	*
	*     filename*=UTF-8''example.txt
	*
	* filename* uses RFC 5987 ext-value syntax.
	* ================================================================
	*/

	/*
	* RFC 5987 attr-char:
	*
	* ALPHA / DIGIT / "!" / "#" / "$" / "&" / "+" /
	* "-" / "." / "^" / "_" / "`" / "|" / "~"
	*/
	static bool isAttrChar(char c) {

		unsigned char uc = static_cast<unsigned char>(c);

		if ((uc >= 'A' && uc <= 'Z') ||
			(uc >= 'a' && uc <= 'z') ||
			(uc >= '0' && uc <= '9')) {
			return true;
		}

		switch (uc) {

		case '!':
		case '#':
		case '$':
		case '&':
		case '+':
		case '-':
		case '.':
		case '^':
		case '_':
		case '`':
		case '|':
		case '~':
		return true;

		default:
		return false;

		}

	}

	/*
	* Validate an RFC 5987 ext-value.
	*
	* Grammar:
	*
	*     ext-value = charset "'" [ language ] "'" value-chars
	*
	* value-chars = *( pct-encoded / attr-char )
	*
	* We return the raw ext-value.  Decoding the charset into Unicode is
	* deliberately not done here because C++98 has no standard charset
	* conversion facility.
	*/
	static bool validateExtValue(const std::string& value) {

		std::size_t firstQuote = value.find('\'');

		if (firstQuote == std::string::npos) return false;

		std::size_t secondQuote = value.find('\'', firstQuote + 1);

		if (secondQuote == std::string::npos) return false;

		/*
		* charset must be non-empty and is a token.
		*/
		if (firstQuote == 0) return false;

		std::size_t i;

		for (i = 0; i < firstQuote; ++i) {
			if (!isTChar(value[i])) return false;
		}

		/*
		* language may be empty.
		*
		* RFC 5646 language-tag syntax is more involved than a token,
		* so we validate the RFC 5987 language component conservatively:
		* ASCII letters/digits and '-'.
		*/
		for (i = firstQuote + 1; i < secondQuote; ++i) {
			char c = value[i];

			if (!((c >= 'A' && c <= 'Z') ||
				(c >= 'a' && c <= 'z') ||
				(c >= '0' && c <= '9') ||
				(c == '-'))) {
				return false;
			}

		}

		/*
		* value-chars.
		*/
		i = secondQuote + 1;
		while (i < value.size()) {

			char c = value[i];

			if (isAttrChar(c)) {
				++i;
				continue;
			}

			/*
			* pct-encoded = "%" HEXDIG HEXDIG
			*/
			if (c == '%') {

				if (i + 2 >= value.size()) return false;

				if (!isHexDigit(value[i + 1]) ||
					!isHexDigit(value[i + 2])) {
					return false;
				}

				i += 3;
				continue;

			}

			return false;

		}

		return true;

	}

	/*
	* Decode RFC 5987 percent encoding.
	*
	* This does NOT convert the declared charset to UTF-8.
	*
	* For example:
	*
	*     UTF-8''%E2%82%AC.txt
	*
	* becomes the raw UTF-8 byte sequence:
	*
	*     E2 82 AC . t x t
	*
	* The charset still needs to be interpreted by the caller.
	*/
	static bool decodeExtValueBytes(const std::string& extValue, std::string& decoded) {

		if (!validateExtValue(extValue)) return false;

		std::size_t firstQuote = extValue.find('\'');
		std::size_t secondQuote = extValue.find('\'', firstQuote + 1);

		decoded.clear();

		std::size_t i = secondQuote + 1;
		while (i < extValue.size()) {

		char c = extValue[i];
		if (c == '%') {

			int hi = hexDigitValue(extValue[i + 1]);
			int lo = hexDigitValue(extValue[i + 2]);

			decoded += static_cast<char>((hi << 4) | lo);

			i += 3;

			} else {

			decoded += c;
			++i;
			}
		}

		return true;

	}

	/*
	* ================================================================
	* Content-Disposition syntax
	* ================================================================
	*
	* The common syntax is:
	*
	*     disposition-type *( ";" disposition-parm )
	*
	* The actual allowed semantics depend on context:
	*
	*     CD_HTTP
	*         RFC 6266
	*
	*     CD_MULTIPART_FORM_DATA
	*         RFC 7578
	* ================================================================
	*/

	static bool parseContentDisposition(const std::string& headerValue,
								 Context context,
								 HTTPParameters::MIMEValue& result) {

		result.value.clear();
		result.parameters.clear();

		std::size_t pos = 0;

		HTTPGrammar::skipOWS(headerValue, pos);

		/*
		* disposition-type = token
		*/
		std::string disposition;

		if (!HTTPGrammar::parseToken(headerValue, pos, disposition)) {
			return false;
		}

		disposition = tolowerASCII(disposition);

		/*
		* RFC 7578:
		*
		* Each multipart/form-data part MUST have
		* Content-Disposition with disposition-type "form-data".
		*/
		if (context == MULTIPART_FORM_DATA &&
			!equalCI(disposition, "form-data")) {
			return false;
		}
		/*
		* For CD_HTTP, disposition is already guaranteed to be a token
		* by parseToken(), and RFC 6266 permits extension disposition
		* types, so no additional validation is necessary here.
		*
		* RFC 6266 allows:
		*
		*     inline
		*     attachment
		*     disp-ext-type
		*
		* where disp-ext-type is a token.
	*
		*/

		result.value = disposition;

		/*
		* Parse parameters.
		*/
		if (!parseParameters(headerValue, pos, result.parameters,
							 HTTPParameters::RFC6266)) {
			result.value.clear();
			result.parameters.clear();
			return false;
		}

	/*
	* ------------------------------------------------------------
	* Duplicate parameter names
	* ------------------------------------------------------------
	*
	* RFC 6266 explicitly treats multiple instances of the same
	* parameter name as invalid.
	*
	* For strict multipart processing, reject duplicates too,
	* rather than letting the application accidentally choose one.
	* ------------------------------------------------------------
	*/

		std::size_t i;
		std::size_t j;

		for (i = 0; i < result.parameters.size(); ++i) {
			for (j = i + 1; j < result.parameters.size(); ++j) {
				if (equalCI(result.parameters[i].name,
							result.parameters[j].name)) {
					return false;
				}
			}
		}

	/*
	* ------------------------------------------------------------
	* RFC 7578 multipart/form-data rules
	* ------------------------------------------------------------
	*/

		if (context == MULTIPART_FORM_DATA) {
		/*
		* name is REQUIRED.
		*/
			std::size_t nameCount = 0;
			for (i = 0; i < result.parameters.size(); ++i) {

				if (equalCI(result.parameters[i].name, "name")) {
					++nameCount;
				}
				/*
				* RFC 7578 explicitly says filename* MUST NOT be used.
				*/
				if (equalCI(result.parameters[i].name, "filename*")) {
					return false;
				}

			}

			if (nameCount != 1) return false;

			return true;
		}

	/*
	* ------------------------------------------------------------
	* RFC 6266 HTTP Content-Disposition rules
	* ------------------------------------------------------------
	*/

		/*
		* Validate filename* if present.
		*/
		for (i = 0; i < result.parameters.size(); ++i) 	{

			if (equalCI(result.parameters[i].name, "filename*")) {
				// if (!validateExtValue(result.parameters[i].value)) {
				std::string decoded;
				if (!decodeExtValueBytes(result.parameters[i].value, decoded)) {
					return false;
				}
				result.parameters[i].value = decoded;
			}
		}

		return true;

	}

	/*
	* ================================================================
	* extractContentDisposition()
	* ================================================================
	*
	* For HTTP:
	*
	*     extractContentDisposition(value,
	*                               HTTP,
	*                               disposition,
	*                               name,
	*                               filename,
	*                               filenameStar,
	*                               ...)
	*
	* For multipart/form-data:
	*
	*     extractContentDisposition(value,
	*                               MULTIPART_FORM_DATA,
	*                               disposition,
	*                               name,
	*                               filename,
	*                               filenameStar,
	*                               ...)
	*
	* filenameStar is returned as the raw RFC 5987 ext-value.
	*
	* If you want the percent-decoded bytes, use
	* decodeExtValueBytes().
	* ================================================================
	*/

	bool extractContentDisposition(const std::string& headerValue,
								Context context,
								std::string& disposition,
								std::string* name,
								std::string* filename,
								std::string* filenameStar,
								std::vector<HTTPParameters::MIMEParameter>* parameters) {

		HTTPParameters::MIMEValue parsed;
		disposition.clear();

		if (name != NULL) name->clear();

		if (filename != NULL) filename->clear();

		if (filenameStar != NULL) filenameStar->clear();

		if (parameters != NULL) parameters->clear();

		if (!parseContentDisposition(headerValue, context, parsed)) {
			return false;
		}

		disposition = parsed.value;
		if (parameters != NULL) *parameters = parsed.parameters;

		for (std::size_t i = 0; i < parsed.parameters.size(); ++i) {

			const HTTPParameters::MIMEParameter& p = parsed.parameters[i];

			if (equalCI(p.name, "name")) {

				if (name != NULL) {

				*name = p.value;
				}

			} else if (equalCI(p.name, "filename")) {

				if (filename != NULL) {
					*filename = p.value;
				}

			} else if (equalCI(p.name, "filename*")) {

				if (filenameStar != NULL) {
					*filenameStar = p.value;
				}

			}

		}

		return true;

	}

	/*
	* ================================================================
	* Convenience overload: no filename*
	* ================================================================
	*/

	bool extractContentDisposition(const std::string& headerValue,
													Context context,
													std::string& disposition,
													std::string* name,
													std::string* filename) {

		return extractContentDisposition(headerValue,
										context,
										disposition,
										name,
										filename,
										NULL,
										NULL);

	}

}
