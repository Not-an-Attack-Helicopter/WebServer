#pragma once

#include "HTTPResponse.hpp"
#include "HTTPRequest.hpp"
#include "Config.hpp"
// #include "utils.hpp"
// #include "types.hpp"

// // In Server or Client
// RequestHandler handler;  // Single instance, shared across all clients
//
// // When a Client has fully parsed a request:
// HTTPRequest request = client->getRequest();
// HTTPResponse response = handler.handle(request);
// client->sendResponse(response);

// class RequestHandler {
// public:
// 	HTTPResponse handle(const HTTPRequest& request);
//
// private:
// 	HTTPResponse _handleCGI(const HTTPRequest& request);
// 	HTTPResponse _handleFileUpload(const HTTPRequest& request);
// 	HTTPResponse _handleStaticFile(const HTTPRequest& request);
// 	HTTPResponse _handleDefault(const HTTPRequest& request);
// };


class RequestHandler {

	public:

		RequestHandler(void);
		~RequestHandler(void);
		RequestHandler(const RequestHandler& other);
		RequestHandler& operator = (const RequestHandler& other);

		void					handle(HTTPRequest* request, HTTPResponse* response, Config* config);
		void					handle(const HTTPRequest& req, HTTPResponse& res, const Config& config);

	private:

		const Config*			_config;

		const HTTPRequest*		_request;

		const LocationConfig*	_location;

		const LocationConfig*	_matchLocation(void) const;

		bool					_methodAllowed(void) const;

		void					_handleRedirect(HTTPRequest* request, HTTPResponse* response);
		void					_handleStatic(HTTPRequest* request, HTTPResponse* response);
		void					_handleAutoindex(HTTPResponse* response, const std::string& dir_path);
		void					_handleUpload(HTTPRequest* request, HTTPResponse* response);
		void					_handleDelete(HTTPRequest* request, HTTPResponse* response);
		void					_errorResponse(HTTPResponse* response, int code);


};
