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

// #include "HTTPResponse.hpp"
// #include "HTTPRequest.hpp"
// #include "Config.hpp"
#include "Client.hpp"
// #include "utils.hpp"

#define dispatch Dispatcher::instance()

class Dispatcher {

public:

	static Dispatcher&					instance(void);

	void								request(Client& client);
	void								errorPage(const Config::Location* location,
												  HTTPResponse& response,
												  bool headers_only,
												  const StatusCode& code);

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
