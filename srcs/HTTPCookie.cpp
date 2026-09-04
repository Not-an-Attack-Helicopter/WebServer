/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPCookie.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 20:47:56 by bstorck           #+#    #+#             */
/*   Updated: 2026/09/04 20:47:57 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/HTTPCookie.hpp"
#include "../incs/HTTPGrammar.hpp"

namespace HTTPCookie {

	static bool isCookieOctet(char c) {

	unsigned char uc = static_cast<unsigned char>(c);

	return uc == 0x21 ||
		(uc >= 0x23 && uc <= 0x2b) ||
		(uc >= 0x2d && uc <= 0x3a) ||
		(uc >= 0x3c && uc <= 0x5b) ||
		(uc >= 0x5d && uc <= 0x7e);
	}

	static bool parseCookieValue(const std::string& s,
								 std::size_t& pos,
								 std::string& value) {

		value.clear();

		if (pos < s.size() && s[pos] == '"') {

			++pos;

			while (pos < s.size() &&
				isCookieOctet(s[pos])) {

				value += s[pos];
				++pos;
			}

			if (pos >= s.size() ||
				s[pos] != '"') {

				return false;
			}

			++pos;
			return true;
		}

		while (pos < s.size() &&
			isCookieOctet(s[pos])) {

			value += s[pos];
			++pos;
		}

		return true;
	}

	bool extractCookies(const std::string& header_value, HTTPRequest& request) {

		std::size_t pos = 0;

		for (;;) {

			HTTPRequest::Cookie cookie;

			/*
			* cookie-name = token
			*/
			if (!HTTPGrammar::parseToken(header_value, pos, cookie.name)) {
				return false;
			}

			/*
			* cookie-pair = cookie-name "=" cookie-value
			*/
			if (pos >= header_value.size() ||
				header_value[pos] != '=') {

				return false;
			}

			++pos;

			/*
			* cookie-value =
			*     *cookie-octet
			*   / ( DQUOTE *cookie-octet DQUOTE )
			*/
			if (!parseCookieValue(header_value, pos, cookie.value)) {
				return false;
			}

			// cookies.push_back(cookie);
			request.setCookie(cookie);

			/*
			* End of cookie-string.
			*/
			if (pos == header_value.size()) {
				return true;
			}

			/*
			* cookie-string =
			*     cookie-pair *( ";" SP cookie-pair )
			*/
			if (header_value[pos] != ';') {
				return false;
			}

			++pos;

			if (pos >= header_value.size() ||
				header_value[pos] != ' ') {

				return false;
			}

			++pos;

			if (pos >= header_value.size()) {
				return false;
			}
		}
	}

}
