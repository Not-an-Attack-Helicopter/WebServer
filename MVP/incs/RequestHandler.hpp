#pragma once

#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
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

		RequestHandler(const Config& server, const HTTPRequest& req);
		~RequestHandler(void);
		RequestHandler(const RequestHandler& other);
		RequestHandler& operator = (const RequestHandler& other);

		void					handler(HTTPResponse* response);

	private:

		const Config&			_server;

		const HTTPRequest&		_req;

		const LocationConfig*	_location;

		const LocationConfig*	_match_location(void) const;

		bool					_method_allowed(void) const;

		void					_handle_redirect(HTTPResponse* response);
		void					_handle_static(HTTPResponse* response);
		void					_handle_autoindex(HTTPResponse* response, const std::string& dir_path);
		void					_handle_upload(HTTPResponse* response);
		void					_handle_delete(HTTPResponse* response);
		void					_error_response(HTTPResponse* response, int code);

		RequestHandler(void);

};
