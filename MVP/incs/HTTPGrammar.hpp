/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPGrammar.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 22:11:52 by bstorck           #+#    #+#             */
/*   Updated: 2026/08/24 22:11:53 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_GRAMMAR_HPP
#define HTTP_GRAMMAR_HPP

#include <cstddef>
#include <string>

namespace HTTPGrammar {

	/*
	* ================================================================
	* ASCII helpers
	* ================================================================
	*/

	char tolowerASCII(char c);

	std::string tolowerASCII(const std::string& s);

	bool equalCI(const std::string& a,
				const std::string& b);

	/*
	* ================================================================
	* RFC 9110 character classes
	* ================================================================
	*/

	/*
	* OWS = *( SP / HTAB )
	*/
	bool isOWS(char c);

	void skipOWS(const std::string& s,
				std::size_t& pos);

	/*
	* tchar
	*
	* tchar = "!" / "#" / "$" / "%" / "&" / "'"
	*       / "*" / "+" / "-" / "." / "^" / "_"
	*       / "`" / "|" / "~" / DIGIT / ALPHA
	*/
	bool isTChar(char c);

	/*
	* token = 1*tchar
	*/
	bool parseToken(const std::string& s,
					std::size_t& pos,
					std::string& token);

	/*
	* VCHAR = %x21-7E
	*/
	bool isVChar(char c);

	/*
	* obs-text = %x80-FF
	*/
	bool isObsText(char c);

	/*
	* qdtext
	*/
	bool isQDText(char c);

	/*
	* quoted-pair character.
	*/
	// bool isQuotedPairChar(char c);

	/*
	* quoted-string
	*
	* quoted-string =
	*     DQUOTE *( qdtext / quoted-pair ) DQUOTE
	*
	* The surrounding DQUOTE characters are removed.
	* quoted-pair escaping is removed.
	*/
	bool parseQuotedString(const std::string& s,
						std::size_t& pos,
						std::string& value);

} // namespace HTTPGrammar

#endif
