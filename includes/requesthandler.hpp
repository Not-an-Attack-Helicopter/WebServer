#pragma once
#include "utils.hpp"
#include "webserver.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

class RequestHandler {
	private:
		const ServerConfig&     _server;
		const HTTPRequest&      _req;
		const LocationConfig*   _location;

		const LocationConfig*   _match_location() const;
		bool                    _method_allowed() const;

		HTTPResponse    _handle_redirect();
		HTTPResponse    _handle_static();
		HTTPResponse    _handle_autoindex(const std::string& dir_path);
		HTTPResponse    _handle_upload();
		HTTPResponse    _handle_delete();
		HTTPResponse    _error_response(int code);
		RequestHandler();
	public:
		RequestHandler(const ServerConfig& server, const HTTPRequest& req);
		RequestHandler(const RequestHandler& other);
		RequestHandler& operator=(const RequestHandler& other);
		~RequestHandler();

		HTTPResponse    handler();
};