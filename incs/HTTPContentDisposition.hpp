/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPContentDisposition.hpp                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 22:13:23 by bstorck           #+#    #+#             */
/*   Updated: 2026/08/24 22:13:24 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef HTTP_CONTENT_DISPOSITION_HPP
# define HTTP_CONTENT_DISPOSITION_HPP

# include "HTTPParameters.hpp"
# include <string>

namespace HTTPContentDisposition {

	enum Context {
		HTTP,
		MULTIPART_FORM_DATA
	};

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

	bool extractContentDisposition(const std::string& headerValue,
								   Context context,
								   std::string& disposition,
								   std::string* name,
								   std::string* filename,
								   std::string* filenameStar,
								   std::vector<HTTPParameters::MIMEParameter>* parameters);

	bool extractContentDisposition(const std::string& headerValue,
								   Context context,
								   std::string& disposition,
								   std::string* name,
								   std::string* filename);

}

#endif

