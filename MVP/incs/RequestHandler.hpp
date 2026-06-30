#pragma once

#include "HTTPResponse.hpp"
#include "HTTPRequest.hpp"
// #include "utils.hpp"
#include "types.hpp"

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

		RequestHandler(const Config& config, const HTTPRequest& request);
		~RequestHandler(void);
		RequestHandler(const RequestHandler& other);
		RequestHandler& operator = (const RequestHandler& other);

		void					handler(HTTPResponse* response);

	private:

		const Config&			_config;

		const HTTPRequest&		_request;

		const LocationConfig*	_location;

		const LocationConfig*	_matchLocation(void) const;

		bool					_methodAllowed(void) const;

		void					_handleRedirect(HTTPResponse* response);
		void					_handleStatic(HTTPResponse* response);
		void					_handleAutoindex(HTTPResponse* response, const std::string& dir_path);
		void					_handleUpload(HTTPResponse* response);
		void					_handleDelete(HTTPResponse* response);
		void					_errorResponse(HTTPResponse* response, int code);

		RequestHandler(void);

};
