/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPParameters.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 22:12:24 by bstorck           #+#    #+#             */
/*   Updated: 2026/08/24 22:12:25 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_PARAMETERS_HPP
#define HTTP_PARAMETERS_HPP

// #include "../incs/HTTPGrammar.hpp"
#include <string>
#include <vector>

namespace HTTPParameters {

	/*
	* ================================================================
	* Data structures
	* ================================================================
	*/

	struct MIMEParameter {

		std::string name;
		std::string value;

		/*
		* true  => parameter-value was a quoted-string
		* false => parameter-value was a token
		*/
		bool quoted;

		MIMEParameter() : quoted(false) {}

	};

	struct MIMEValue {
		/*
		* Content-Type:
		*
		*     "text/plain"
		*
		* Content-Disposition:
		*
		*     "form-data"
		*     "attachment"
		*     "inline"
		*/
		std::string value;

		std::vector<MIMEParameter> parameters;
	};

	enum ParameterSyntax {
		RFC9110,
		RFC6266
	};

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

	bool parseParameters(const std::string& s, std::size_t& pos,
						 std::vector<MIMEParameter>& parameters,
						 ParameterSyntax syntax);

	/*
	* ================================================================
	* Parameter lookup
	* ================================================================
	*/

	// const MIMEParameter* findParameter(const std::vector<MIMEParameter>& parameters,
	// 								   const char* name);

	/*
	* Find a parameter and make sure it occurs exactly once.
	*
	* This is useful for fields where duplicate instances are invalid
	* or where the application must not ambiguously choose one.
	*/

	bool getUniqueParameter(const std::vector<MIMEParameter>& parameters,
							const char* name,
							std::string& value);

}

#endif
