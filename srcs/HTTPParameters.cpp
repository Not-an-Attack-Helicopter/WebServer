/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPParameters.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 22:35:07 by bstorck           #+#    #+#             */
/*   Updated: 2026/08/24 22:35:09 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/HTTPParameters.hpp"
#include "../incs/HTTPGrammar.hpp"
#include "../incs/utils.hpp"

namespace HTTPParameters {

	/*
	* ================================================================
	* parameter-value
	* ================================================================
	*
	* parameter-value = token / quoted-string
	* ================================================================
	*/

	static bool parseParameterValue(const std::string& s,
									std::size_t& pos,
									std::string& value,
									bool& quoted) {

		value.clear();
		quoted = false;

		if (pos >= s.size()) return false;

		if (s[pos] == '"') {
			quoted = true;

			return HTTPGrammar::parseQuotedString(s, pos, value);
		}

		return HTTPGrammar::parseToken(s, pos, value);

	}

	/*
	* ================================================================
	* parseMIMEparameter()
	* ================================================================
	*
	* Parses the parameter portion:
	*
	*     ; name=value
	*     ; name="value"
	*     ; charset=UTF-8; boundary=abc
	*
	* It does NOT parse the media type or disposition type itself.
	*
	* 'pos' must point at either:
	*
	*     end of string
	*
	* or:
	*
	*     ';'
	*
	* RFC 9110 parameter syntax is:
	*
	*     parameters = *( OWS ";" OWS [ parameter ] )
	*     parameter  = parameter-name "=" parameter-value
	*
	* The parameter itself is optional after ';'.
	*
	* Therefore, under RFC 9110 syntax:
	*
	*     text/plain;
	*     text/plain;;
	*     text/plain; ; charset=UTF-8
	*
	* are permitted by the generic parameter grammar.
	*
	* RFC 6266 uses a different grammar:
	*
	*     disposition-type *( ";" disposition-parm )
	*
	* where disposition-parm is not optional. Therefore, for
	* RFC 6266 syntax, an actual parameter is required after ';'.
	* ================================================================
	*/

	bool parseParameters(const std::string& s,
						std::size_t& pos,
						std::vector<MIMEParameter>& parameters,
						ParameterSyntax syntax) {

		parameters.clear();

		// while (true) {
		for (;;) {

			HTTPGrammar::skipOWS(s, pos);

			/*
			* No more input.
			*/
			if (pos == s.size()) return true;

			/*
			* Every parameter must begin with ';'.
			*/
			if (s[pos] != ';') return false;

			++pos;

			HTTPGrammar::skipOWS(s, pos);

			// /*
			// * No empty parameter.
			// */
			if (syntax == RFC9110) {

				/*
				* RFC 9110:
				*
				* parameters = *( OWS ";" OWS [ parameter ] )
				*
				* parameter is optional.
				*/
				if (pos == s.size() || s[pos] == ';') continue;

			} else {

				/*
				* RFC 6266:
				*
				* a disposition parameter is required after ';'.
				*/
				if (pos == s.size()) return false;
			}

			/*
			* parameter-name = token
			*/
			std::string name;
			if (!HTTPGrammar::parseToken(s, pos, name)) return false;

			/*
			* IMPORTANT:
			*
			* Do NOT skip OWS here.
			*
			* The parameter grammar is:
			*
			*     parameter = parameter-name "=" parameter-value
			*
			* Therefore:
			*
			*     foo=bar    valid
			*     foo =bar   invalid
			*     foo= bar   invalid
			*/
			if (pos >= s.size() || s[pos] != '=') return false;

			++pos;

			/*
			* parameter-value
			*/
			std::string value;
			bool quoted;

			if (!parseParameterValue(s, pos, value, quoted)) {
				return false;
			}

			MIMEParameter parameter;

			/*
			* Parameter names are case-insensitive.
			*/
			// parameter.name = lowerASCIIString(name);
			parameter.name = name;
			parameter.value = value;
			parameter.quoted = quoted;

			parameters.push_back(parameter);

			/*
			* The next thing must be:
			*
			*     OWS
			*     ';'
			*
			* or end of input.
			*/
			HTTPGrammar::skipOWS(s, pos);

			if (pos == s.size()) return true;

			if (s[pos] != ';') return false;

		}

	}

	/*
	* ================================================================
	* Parameter lookup
	* ================================================================
	*/

	// static const MIMEParameter* findParameter(const std::vector<MIMEParameter>& parameters,
	// 										  const char* name) {
 //
	// 	std::size_t i;
 //
	// 	for (i = 0; i < parameters.size(); ++i) {
	// 		if (equalCI(parameters[i].name,name)) {
	// 		return &parameters[i];
	// 		}
	// 	}
 //
	// 	return NULL;
 //
	// }

	/*
	* Find a parameter and make sure it occurs exactly once.
	*
	* This is useful for fields where duplicate instances are invalid
	* or where the application must not ambiguously choose one.
	*/
	bool getUniqueParameter(const std::vector<MIMEParameter>& parameters,
							const char* name,
							std::string& value) {

		std::size_t count = 0;

		value.clear();

		for (std::size_t i = 0; i < parameters.size(); ++i) {

			if (equalCI(parameters[i].name, name)) {

				++count;

				if (count == 1) {
					value = parameters[i].value;
				}

			}
		}

		return count == 1;

	}

}
