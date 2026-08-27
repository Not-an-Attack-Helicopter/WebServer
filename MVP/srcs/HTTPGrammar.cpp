/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPGrammar.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 22:34:59 by bstorck           #+#    #+#             */
/*   Updated: 2026/08/24 22:35:01 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/HTTPGrammar.hpp"

namespace HTTPGrammar {

	/*
	* ================================================================
	* ASCII helpers
	* ================================================================
	*/

	char tolowerASCII(char c) {
		unsigned char uc =
			static_cast<unsigned char>(c);

		if (uc >= static_cast<unsigned char>('A') &&
			uc <= static_cast<unsigned char>('Z')) {

			uc = static_cast<unsigned char>(
				uc + ('a' - 'A'));
		}

		return static_cast<char>(uc);
	}

	std::string tolowerASCII(const std::string& s) {
		std::string result(s);

		std::size_t i;

		for (i = 0; i < result.size(); ++i) {
			result[i] = tolowerASCII(result[i]);
		}

		return result;
	}

	bool equalCI(const std::string& a,
				const std::string& b) {
		if (a.size() != b.size()) {
			return false;
		}

		std::size_t i;

		for (i = 0; i < a.size(); ++i) {

			if (tolowerASCII(a[i]) !=
				tolowerASCII(b[i])) {

				return false;
			}
		}

		return true;
	}

	/*
	* ================================================================
	* RFC 9110 character classes
	* ================================================================
	*/

	/*
	* RFC 9110:
	*
	* OWS = *( SP / HTAB )
	*/
	bool isOWS(char c) {
		return c == ' ' || c == '\t';
	}

	void skipOWS(const std::string& s,
				std::size_t& pos) {
		while (pos < s.size() &&
			isOWS(s[pos])) {

			++pos;
		}
	}

	/*
	* RFC 9110:
	*
	* tchar = "!" / "#" / "$" / "%" / "&" / "'"
	*       / "*" / "+" / "-" / "." / "^" / "_"
	*       / "`" / "|" / "~" / DIGIT / ALPHA
	*/
	bool isTChar(char c) {
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
		case '%':
		case '&':
		case '\'':
		case '*':
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
	* RFC 9110:
	*
	* token = 1*tchar
	*/
	bool parseToken(const std::string& s,
					std::size_t& pos,
					std::string& token) {

		std::size_t start = pos;

		while (pos < s.size() &&
			isTChar(s[pos])) {

			++pos;
		}

		if (pos == start) {
			return false;
		}

		token.assign(s,
					start,
					pos - start);

		return true;
	}

	/*
	* RFC 9110:
	*
	* VCHAR = %x21-7E
	*/
	bool isVChar(char c) {
		unsigned char uc =
			static_cast<unsigned char>(c);

		return uc >= 0x21 &&
			uc <= 0x7e;
	}

	/*
	* RFC 9110:
	*
	* obs-text = %x80-FF
	*/
	bool isObsText(char c) {
		unsigned char uc =
			static_cast<unsigned char>(c);

		return uc >= 0x80;
	}

	/*
	* RFC 9110:
	*
	* qdtext =
	*       HTAB
	*     / SP
	*     / %x21
	*     / %x23-5B
	*     / %x5D-7E
	*     / obs-text
	*/
	bool isQDText(char c) {
		unsigned char uc =
			static_cast<unsigned char>(c);

		if (uc == '\t' ||
			uc == ' ') {
			return true;
		}

		if (uc == 0x21) {
			return true;
		}

		if (uc >= 0x23 &&
			uc <= 0x5b) {
			return true;
		}

		if (uc >= 0x5d &&
			uc <= 0x7e) {
			return true;
		}

		if (uc >= 0x80) {
			return true;
		}

		return false;
	}

	/*
	* RFC 9110:
	*
	* quoted-pair =
	*     "\" ( HTAB / SP / VCHAR / obs-text )
	*/
	static bool isQuotedPairChar(char c) {
		return c == '\t' ||
			c == ' ' ||
			isVChar(c) ||
			isObsText(c);
	}

	/*
	* ================================================================
	* RFC 9110 quoted-string
	* ================================================================
	*
	* quoted-string =
	*     DQUOTE *( qdtext / quoted-pair ) DQUOTE
	* ================================================================
	*/

	bool parseQuotedString(const std::string& s,
						std::size_t& pos,
						std::string& value) {
		value.clear();

		if (pos >= s.size() ||
			s[pos] != '"') {

			return false;
		}

		++pos;

		while (pos < s.size()) {

			char c = s[pos];

			/*
			* Closing DQUOTE.
			*/
			if (c == '"') {

				++pos;
				return true;
			}

			/*
			* quoted-pair.
			*/
			if (c == '\\') {

				++pos;

				if (pos >= s.size()) {
					return false;
				}

				c = s[pos];

				if (!isQuotedPairChar(c)) {
					return false;
				}

				value += c;
				++pos;

				continue;
			}

			/*
			* qdtext.
			*/
			if (!isQDText(c)) {
				return false;
			}

			value += c;
			++pos;
		}

		/*
		* Unterminated quoted-string.
		*/
		return false;
	}

} // namespace HTTPGrammar
