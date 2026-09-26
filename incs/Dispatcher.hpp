/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Dispatcher.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/11 15:42:07 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/11 15:42:10 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#define dispatcher Dispatcher::instance()

// #include "HTTPResponse.hpp"
// #include "HTTPRequest.hpp"
// #include "Config.hpp"
#include "Client.hpp"
// #include "utils.hpp"

class Dispatcher {

public:

	static Dispatcher&					instance(void);

	void								handleRequest(Client& client);
	void								buildErrorResponse(const StatusCode& code,
														   const Config::Location* location,
														   bool headers_only,
														   HTTPResponse& response);

	static const Config::Location*		resolveLocation(const std::vector<Config::Location>& locations,
														const std::string& requested_location_path);

	typedef std::map<std::string, std::string> content_type_map;

	static content_type_map				initContentTypeMap(void);

private:

	Dispatcher(void);
	~Dispatcher(void);
	Dispatcher(const Dispatcher& other);
	Dispatcher& operator = (const Dispatcher& other);

};

#endif
