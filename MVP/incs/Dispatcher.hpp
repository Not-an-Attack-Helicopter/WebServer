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

#define dispatcher Dispatcher::instance()

class Dispatcher {

public:

	static Dispatcher&					instance(void);

	void								handleRequest(Client& client);

	static const Config::Location*		matchLocation(const std::vector<Config::Location>& locations,
													  const std::string& requested_location_path);

	typedef std::map<std::string, std::string> content_type_map;

	static content_type_map				initContentTypeMap(void);

private:

	Dispatcher(void);
	~Dispatcher(void);
	Dispatcher(const Dispatcher& other);
	Dispatcher& operator = (const Dispatcher& other);

};

// #include "templates.tpp"

#endif


// // In Server or Client
// Dispatcher handler;  // Single instance, shared across all clients
//
// // When a Client has fully parsed a request:
// HTTPRequest request = client->getRequest();
// HTTPResponse response = handler.handle(request);
// client->sendResponse(response);

// class Dispatcher {
// public:
// 	HTTPResponse handle(const HTTPRequest& request);
//
// private:
// 	HTTPResponse _handleCGI(const HTTPRequest& request);
// 	HTTPResponse _handleFileUpload(const HTTPRequest& request);
// 	HTTPResponse _handleStaticFile(const HTTPRequest& request);
// 	HTTPResponse _handleDefault(const HTTPRequest& request);
// };

// typedef struct s_ctx {
// 	const Config* config;
// 	const Location* location;
// 	const HTTPRequest* request;
// 	HTTPResponse* response;
// 	std::string path;
// } t_ctx;

// struct s_ctx {
// 	const Config* config;
// 	const Location* location;
// 	const HTTPRequest* request;
// 	HTTPResponse* response;
// 	std::string path;
// };

// const Config&			_config;

// const HTTPRequest&		_request;

// const Location&	_location;

// void						handleGet(const HTTPRequest& request,
// 									  HTTPResponse& response);
// void						handlePost(const HTTPRequest& request,
// 									   HTTPResponse& response);
// void						handlePut(const HTTPRequest& request,
// 									  HTTPResponse& response);
// Breaks down with many endpoints!!!

// const Location*				_matchLocation(const Config* config,
// 										   const std::string& path) const;

// std::string					_matchMethod(const Location* location,
// 										 const HTTPRequest* request) const;

// std::string					_matchContentType(const std::string& path);

// void						_handleRedirect(const Location* location,
// 											HTTPResponse* response);

// void						_serveFile(const Config* config,
// 									   const Location* location,
// 									   HTTPResponse* response,
// 									   const std::string& path);

// void						_serveDirectoryListing(const Config* config,
// 												   const Location* location,
// 												   HTTPResponse* response,
// 												   const std::string& path);

// void						_handleUpload(Client& client, // TODO
// 										  const Location& location);

// void						_handleGet(Client& client, // TODO
// 									   const Location& location);

// void						_handlePost(Client& client, // TODO
// 										const Location& location);

// void						_handleDelete(Client& client, // TODO
// 										  const Location& location);

// void						_serveErrorPage(const Config* config,
// 											const Location* location,
// 											HTTPResponse* response,
// 											int code);
