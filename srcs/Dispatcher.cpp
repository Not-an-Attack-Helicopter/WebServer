/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Dispatcher.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/11 15:42:24 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/11 15:42:35 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/Dispatcher.hpp"
// #include "../incs/HTTPResponse.hpp"
// #include "../incs/HTTPRequest.hpp"
// #include "../incs/HTTPGrammar.hpp"
#include "../incs/constexpr.hpp"
#include "../incs/templates.hpp"
// #include "../incs/Config.hpp"
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"
#include "../incs/CgiHandler.hpp"
// #include <sys/stat.h>	// stat
// #include <sys/wait.h>	// waitpid
#include <dirent.h>		// opendir, readdir, closedir
#include <unistd.h>
// #include <fcntl.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstddef>
// #include <cstdlib>

static bool isReadable(const std::string& path) {

	if (!isRegularFile(path)) {
		return false;
	}

	return access(path.c_str(), R_OK) == 0;

}

static std::string findIndexFile(const Config::Location& location,
								 const std::string& path) {

	for (std::size_t i = 0; i < location.index_files.size(); ++i) {

		std::string index_file_path = path + location.index_files[i];
		return index_file_path;

	}

	return "";

}

static std::string extractDomainName(const std::string& host_header) {

	std::size_t colonPos = host_header.find(':');
	if (colonPos != std::string::npos) {
		return host_header.substr(0, colonPos); // Strip port
	}
	return host_header; // No port present

}

static const Config::Domain* resolveDomain(const std::vector<Config::Domain>& domains,
										   const std::string& host_header) {

	// Return 400 Bad Request if no domain name provided
	if (host_header.empty()) {
		log.error("dispatch error: empty header");
		return NULL;
	}
	std::string requested_domain_name = extractDomainName(host_header);
	// Looking for exact match
	for (std::size_t i = 0; i < domains.size(); ++i) {
		for (std::size_t j = 0; j < domains[i].names.size(); ++j) {
			if (domains[i].names[j] == requested_domain_name) {
				return &domains[i];
			}
		}
	}
	return NULL;
}

static  Method resolveMethod(const HTTPRequest& request) {

	const std::vector<Method>& config_methods = request.resolved.location->methods;
	const Method& requested_method = request.getMethod();

	for (std::size_t i = 0; i < config_methods.size(); ++i) {

		if (requested_method == config_methods[i]) {
			return requested_method;
		}
	}
	log.error("dispatch error: no method found");
	return METHOD_COUNT;
}

static bool decodeURL(const std::string& input, std::string& result) {

	result.clear();

	std::size_t i = 0;

	while (i < input.size()) {

		if (input[i] != '%') {
			result += input[i];
			++i;
			continue;
		}

		if (i + 2 >= input.size())
			return false;

		if (!isHexDigit(input[i + 1]) ||
			!isHexDigit(input[i + 2]))
			return false;

		int hi = hexDigitValue(input[i + 1]);
		int lo = hexDigitValue(input[i + 2]);

		result += static_cast<char>((hi << 4) | lo);

		i += 3;
	}

	return true;
}

static bool normalizePath(const std::string& path, std::string& result) {

	std::string part;
	std::vector<std::string> parts;
	for (std::size_t i = 0; i <= path.size(); ++i) {

		if (i == path.size() || path[i] == '/') {

			if (part == "..") {
				if (parts.empty()) {
					return false;
				}
				parts.pop_back();
			}

			else if (!part.empty() && part != ".") {
				parts.push_back(part);
			}

			part.clear();

		}

		else {
			part += path[i];
		}

	}

	result = "/";
	for (std::size_t i = 0; i < parts.size(); ++i) {

		if (i != 0) {
			result += "/";
		}
		result += parts[i];

	}

	// Preserve trailing slash.
	if (result != "/" && path.size() > 1 &&
		path[path.size() - 1] == '/') {

		result += "/";

	}

	return true;

}

static std::string matchContentType(const std::string& path) {

	static const Dispatcher::content_type_map content_types = Dispatcher::initContentTypeMap();
	std::size_t dot = path.rfind('.');
	if (dot == std::string::npos || dot == path.size() - 1) {
		return "application/octet-stream";
	} else {
		std::string ext = path.substr(dot);
		if (content_types.find(ext) != content_types.end()) {
			return content_types.at(ext);
		} else {
			return "application/octet-stream";
		}
	}

}

static StatusCode serveFile(const std::string& path,
							const HTTPRequest& request,
							HTTPResponse& response) {

	if (!isReadable(path)) {
		return FORBIDDEN;
	}
	std::ifstream file;
	file.open(path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		return FORBIDDEN;
	}

	file.close();

	std::string content_type = matchContentType(path);

	response.setStatus(OK);
	response.setHeader("Connection", "keep-alive");

	response.setBody(path, DISK, content_type, request.headers_only);

	return OK;

}

static StatusCode removeFile(const std::string& path,
							 HTTPResponse& response) {

	if (std::remove(path.c_str()) != 0) {
		return INTERNAL_SERVER_ERROR;
	}
	log.info("Deleted " + path);

	response.setStatus(NO_CONTENT);
	response.setHeader("Connection", "keep-alive");

	return NO_CONTENT;

}

// static StatusCode prepareCGI(HTTPRequest& request, HTTPResponse& response) {
// 	return NO_STATUS;
// }

static StatusCode serveDirectoryListing(const std::string& path,
										bool supports_delete,
										const HTTPRequest& request,
										HTTPResponse& response) {

	DIR* dir = opendir(path.c_str());

	if (!dir) {
		log.error("dispatch error: could not open directory, permission denied");
		return FORBIDDEN;
	}

	response.setStatus(OK);
	response.setHeader("Connection", "keep-alive");

	std::ostringstream body;
	body	<< tag::DOC << tag::HTML << tag::HEAD << define::META << define::FAVICON << define::STYLE
			<< tag::TITLE << "Index of" << http::_ << request.getPath() << tag::_TITLE
			<< tag::_HEAD << tag::BODY
			<< tag::H1 << "Index of" << http::_ << request.getPath() << tag::_H1;

	struct dirent* entry;

	body	<< tag::UL;
	while ((entry = readdir(dir)) != NULL) {

		std::string name = entry->d_name;
		if (name == ".") {
			continue;
		}

		body	<< tag::LI << tag::A << tag::HREF << name << tag::_HREF;

		if (name == "..") {
			body	<< "Parent Directory";
		} else {
			body	<< name;
		}

		body	<< tag::_A;

		if (supports_delete == true && name != "..") {
			body	<< tag::TAB << button::DELETE_ << request.getPath() << name << button::_DELETE;
		}

		body	<< tag::_LI << tag::BR;

	}
	body	<< tag::_UL;

	if (supports_delete == true) {
		body	<< button::SCRIPT;
	}

	body	<< tag::_BODY << tag::_HTML;

	closedir(dir);

	response.setBody(body.str(), HEAP, "text/html", request.headers_only);
	return OK;

}

static StatusCode handleRedirect(const std::string& path,
								 const HTTPRequest& request,
								 HTTPResponse& response) {

	std::string new_path = path + "/";

	response.setStatus(MOVED_PERMANENTLY);
	response.setHeader("Connection", "keep-alive");
	response.setHeader("location", new_path);

	std::ostringstream body;
	body << "Moved Permanently. Redirecting to " + new_path;
	response.setBody(body.str(), HEAP, "text/plain", request.headers_only);

	return MOVED_PERMANENTLY;

}

static StatusCode handleRedirect(const Config::Location& location,
								 const HTTPRequest& request,
								 HTTPResponse& response) {

	response.setStatus(MOVED_PERMANENTLY);
	response.setHeader("Connection", "keep-alive");
	response.setHeader("Location", location.redirect);

	std::ostringstream body;
	body << "Moved Permanently. Redirecting to " + location.redirect;
	response.setBody(body.str(), HEAP, "text/plain", request.headers_only);

	return MOVED_PERMANENTLY;

}

static StatusCode handleGET(const HTTPRequest& request,
							HTTPResponse& response) {

	const Config::Location& location = *request.resolved.location;
	const std::string& path = request.resolved.path;

	// Only redirect GET requests missing trailing slash
	// (browsers need it for relative links)
	if (path[path.size() - 1] != '/') {
		return handleRedirect(request.getPath(), request, response);
	}

	// Check if index file present
	std::string index_file_path = findIndexFile(location, path);

	// Return index file
	if (!index_file_path.empty() && isReadable(index_file_path)) {
		return serveFile(index_file_path, request, response);

	// No index file found, check if autoindex is enabled
	// autoindex is on, generate directory listing
	} else if (location.autoindex) {
		bool supports_delete = false;
		for (std::size_t i = 0; i < location.methods.size(); ++i) {
			if (location.methods[i] == DELETE) supports_delete = true;
		}
		return serveDirectoryListing(path, supports_delete, request, response);

	// autoindex is off, return 403
	} else {
		return FORBIDDEN;
	}

}

// static StatusCode handleCGI(HTTPRequest& request, HTTPResponse& response) {
// 	return NO_STATUS;
// }

static StatusCode handlePUT(HTTPRequest& request,
							HTTPResponse& response) {

	response.setHeader("Connection", "keep-alive");
	response.setBody("Uploaded\n", HEAP, "text/plain", request.headers_only);

	if (request.created_file) {
		response.setStatus(CREATED);
		return CREATED;
	} else {
		response.setStatus(OK);
		return OK;
	}
}

static StatusCode handlePOST(const HTTPRequest& request,
							 HTTPResponse& response) {

	response.setStatus(CREATED);
	response.setHeader("Connection", "keep-alive");

	response.setBody("Uploaded\n", HEAP, "text/plain", request.headers_only);

	return CREATED;

}

static StatusCode handleRegularFile(HTTPRequest& request,
									HTTPResponse& response) {

	const Method& method = request.resolved.method;
	const std::string& path = request.resolved.path;

	StatusCode status_code = NO_STATUS;

	switch (method) {
		case GET:
			return serveFile(path, request, response);
		case POST:
			return METHOD_NOT_ALLOWED;
		case DELETE:
			return removeFile(path, response);
		case PUT:
			status_code = removeFile(path, response);
			if (status_code >= BAD_REQUEST) {
				return status_code;
			}
			if (request.body.size > request.resolved.location->client_max_body_size) {
				log.warn("payload size exceeds the maximum allowed");
				return PAYLOAD_TOO_LARGE;
			}
			request.parsing.state = HTTPRequest::READING_BODY;
			return NO_STATUS;
		case HEAD:
			return serveFile(path, request, response);
		default:
			return NOT_IMPLEMENTED;
	}

}

static StatusCode handleDirectory(HTTPRequest& request,
								  HTTPResponse& response) {

	const Method& method = request.resolved.method;

	switch (method) {
	case GET:
		return handleGET(request, response);
	case POST:
		if (request.body.size > request.resolved.location->client_max_body_size) {
			log.warn("payload size exceeds the maximum allowed");
			return PAYLOAD_TOO_LARGE;
		}
		request.parsing.state = HTTPRequest::READING_BODY;
		return NO_STATUS;
	case DELETE:
		return METHOD_NOT_ALLOWED;
	case PUT:
		if (request.body.size > request.resolved.location->client_max_body_size) {
			log.warn("payload size exceeds the maximum allowed");
			return PAYLOAD_TOO_LARGE;
		}
		request.parsing.state = HTTPRequest::READING_BODY;
		request.created_file = true;
		return NO_STATUS;
	case HEAD:
		return handleGET(request, response);
	default:
		return NOT_IMPLEMENTED;
	}

}

// argv for execve
static StatusCode routeRequest(HTTPRequest& request,
							   HTTPResponse& response) {

	// // Hand request to CGI
	// if (request.requires_CGI) {
 //
	// 	std::vector<std::string> cgi_args = buildCgiArgs(request);
	// 	(void)cgi_args; // WIP
 //
	// 	// same deal as PUT/POST, gotta spool the body to disk first
	// 	if (!request.is_multipart &&
	// 		(request.body.size != 0 || request.body_chunked)) {
	// 		if (request.body.size > request.resolved.location->client_max_body_size) {
	// 			log.warn("payload size exceeds the maximum allowed");
	// 			return PAYLOAD_TOO_LARGE;
	// 		}
	// 		createFile(request);
	// 	}
	// 	request.parsing.state = HTTPRequest::READING_BODY;

		// return handleCGI(request, response);

	// Match CGI extensions
	if (hasCGIExtension(request)) {
		request.requires_CGI = true;
		// TODO we need to put setting up all things CGI here! The cgi pipes need to be ready to be written to during READING_BODY
		if (request.body.size != 0 ||
			request.body_chunked == true) {
			request.parsing.state = HTTPRequest::READING_BODY;
		} else {
			request.parsing.state = HTTPRequest::CGI_PROCESSING;
		}
		return NO_STATUS;

	// Check if request path exists as static file in `root`
	} else if (isRegularFile(request.resolved.path)) {

		return handleRegularFile(request, response);

	// Check if request is for a directory
	} else if (isDirectory(request.resolved.path) || request.resolved.method == PUT) {

		return handleDirectory(request, response);

	// If not found, send not found
	} else {

		return NOT_FOUND;

	}

}

static StatusCode resolveRoute(Client& client) {

	const Config::Socket& socket = client.getConfig();
	HTTPResponse& response = client.getCurrentResponse();
	HTTPRequest& request = client.getCurrentRequest();
	const std::string& version = request.getVersion();

	// Check HTTP version
	const std::string* connection = request.getHeader("connection");
	if (version == http::V_1_1) {
		if (connection != NULL && *connection == "close") {
			response.setHeader("Connection", "close");
			client.markForTermination();
		}
	} else if (version ==  http::V_1_0) {
		if (connection != NULL && *connection == "keep-alive") {
			response.setHeader("Connection", "close");
			client.markForTermination();
		}
	}

	// Match domain by name
	request.resolved.domain = resolveDomain(socket.domains, *request.getHeader("host"));
	if (!request.resolved.domain) {
		return BAD_REQUEST;
	}

	// Match location by path
	request.resolved.location = Dispatcher::resolveLocation(request.resolved.domain->locations,
															request.getPath());
	if (!request.resolved.location) {
		return NOT_FOUND;
	}
	if (!(*request.resolved.location).redirect.empty()) {
		return handleRedirect((*request.resolved.location), request, response);
	}

	// Check if method allowed
	request.resolved.method = resolveMethod(request);
	if (request.resolved.method == METHOD_COUNT) {
		return METHOD_NOT_ALLOWED;
	}

	// Decode and normalize path, then check for traversal attempts
	// (verify that the resulting path remains inside location's root)
	std::string decoded;
	if (!decodeURL(request.getPath(), decoded)) {
		log.error("dispatch error: malformed target URL");
		return BAD_REQUEST;
	}
	std::string normalized;
	if (!normalizePath(decoded, normalized)) {
		log.error("dispatch error: forbidden path");
		return NOT_FOUND;
	}

	// Create absolute path from root or alias
	if (!request.resolved.location->root.empty()) {
		request.resolved.path = request.resolved.location->root + normalized;
	} else {
		request.resolved.path = request.resolved.location->alias +
		normalized.substr(request.resolved.location->path.size());
	}
	log.debug("absolute path: " + request.resolved.path);

	return NO_STATUS;

}

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Instance	*/
Dispatcher& Dispatcher::instance(void) {
	static Dispatcher instance;
	return instance;
}

// Processing flow
// ---------------
// 1. Match location by path
// ↓
// 2. Check if `return` directive exists
// → YES: Send redirect/status, STOP
// → NO: Continue
// ↓
// 3. Validate `allow_methods`
// → NOT in whitelist: Send 405, STOP
// → IN whitelist: Continue
// ↓
// 4. Check if request path matches `cgi_ext`
// → YES: Execute CGI script, STOP
// → NO: Continue
// ↓
// 5. Check if request is for a directory
// ↓
// 5.1. If request method "GET"
// ↓
// 5.1.1 Check if directory has matching `index` file
// → Found: Serve file, STOP
// → Not found: Use autoindex result
// ↓
// 5.1.2. Check `autoindex` directive
// → ON: List files (if no index found)
// → OFF: Return 403 Forbidden
// ↓
// 5.2. If request method "POST"
// → Handle upload
// ↓
// 5.3. If request method "DELETE"
// → Handle delete
// ↓
// 6. Check if request path exists as static file in `root`
// → Exists: Serve file, STOP
// → Not found: Return 404
// ↓
// 7. If error occurred, check `error_page` directive
// → Matches status code: Serve error page
// → No match: Send default error page

// 				FILE       DIRECTORY
// GET          ✓          ✓
// POST         405        ✓
// DELETE       ✓          405
// PUT          ✓          405
// HEAD         ✓          ✓

// READING_REQUEST_LINE,	X
// READING_HEADERS,			X
// READING_BODY,			X
// DISPATCHING,				✓
// FINALIZING,				✓
// COMPLETE,				X
// ERROR					✓

void Dispatcher::request(Client& client) {

	StatusCode status_code = NO_STATUS;

	HTTPResponse& response = client.getCurrentResponse();
	HTTPRequest& request = client.getCurrentRequest();

	switch (request.parsing.state) {
	case HTTPRequest::ERROR:
		client.markForTermination();
		client.setState(Client::PENDING_RESPONSE);
		return errorPage(request.resolved.location,
						 response,
						 request.headers_only,
						 request.parsing.error_cause);
	case HTTPRequest::COMPLETE:
		// requires_CGI never reaches here -- see parseIncomingData(), a CGI
		// request always lands on CGI_PROCESSING instead once its body is in.
		if (request.resolved.method == PUT) {
			status_code = handlePUT(request, response);
		} else if (request.resolved.method == POST) {
			status_code = handlePOST(request, response);
		} else {
			status_code = BAD_REQUEST;
		}
		if (status_code < BAD_REQUEST) {
			client.setState(Client::PENDING_RESPONSE);
			return;
		}
		break;
	case HTTPRequest::RESOLVING_ROUTE:
		status_code = resolveRoute(client);
		if (status_code == NO_STATUS) {
			status_code = routeRequest(request, response);
		}
		if (status_code < BAD_REQUEST) {
			if (request.parsing.state == HTTPRequest::READING_BODY) {
				client.setState(Client::RECEIVING_BODY);
			} else if (request.parsing.state == HTTPRequest::CGI_PROCESSING) {
				// no body, so routeRequest() already put us straight into
				// CGI_PROCESSING -- matches the state parseIncomingData()
				// lands on once a WITH-body CGI request finishes reading.
				// Server doesn't drive AWAITING_CGI_RESPONSE yet, so this
				// is correctly blocked, not actually running the CGI.
				client.setState(Client::AWAITING_CGI_RESPONSE);
			} else {
				client.setState(Client::PENDING_RESPONSE);
			}
			return;
		}
		break;
	case HTTPRequest::CGI_PROCESSING:
		status_code = handleCGI(request, response, client.getConfig());
		if (status_code < BAD_REQUEST) {
			return;
		}
		break;
	default:
		return;
	}

	if (status_code == BAD_REQUEST ||
		status_code == REQUEST_TIMEOUT ||
		status_code == LENGTH_REQUIRED ||
		status_code >= INTERNAL_SERVER_ERROR) {
		client.markForTermination();
	} else if (status_code == PAYLOAD_TOO_LARGE) {
		client.blockFromReceiving();
	}
	errorPage(request.resolved.location,
			  response,
			  request.headers_only,
			  status_code);

	client.setState(Client::PENDING_RESPONSE);
	return;

}

void Dispatcher::errorPage(const Config::Location* location,
						   HTTPResponse& response,
						   bool headers_only,
						   const StatusCode& code) {

	// Check location error_page first, then server error_page
	std::string error_page_path;
	if (location != NULL && !location->error_pages.empty()) {
		std::map<int, std::string>::const_iterator it = location->error_pages.find(static_cast<int>(code));
		if (it != location->error_pages.end()) {
			error_page_path = it->second;
		}
	}

	response.setStatus(code);
	if (code == BAD_REQUEST ||
		code == REQUEST_TIMEOUT ||
		code == LENGTH_REQUIRED ||
		code == PAYLOAD_TOO_LARGE ||
		code >= INTERNAL_SERVER_ERROR) {
		response.setHeader("Connection", "close");
	} else {
		response.setHeader("Connection", "keep-alive");
	}

	if (!error_page_path.empty()) {

		response.setBody(error_page_path, DISK, "text/html", headers_only);

	} else {

		std::ostringstream body ;
		body	<< tag::HTML << tag::BODY << tag::H1 << "Error" << http::_ << i2a(code) << ":"
				<< http::_ << response.getStatusReason() << tag::_H1 << tag::_BODY << tag::_HTML;
		response.setBody(body.str(), HEAP, "text/html", headers_only);

	}

	return;

}

static bool startsWith(const std::string& requested_path,
					   const std::string& config_location_path,
					   std::size_t requested_location_path_len,
					   std::size_t config_location_path_len) {

	if (config_location_path_len > requested_location_path_len) {
		return false;
	}

	return requested_path.compare(0, config_location_path_len, config_location_path) == 0;

}

const Config::Location* Dispatcher::resolveLocation(const std::vector<Config::Location>& locations,
													const std::string& requested_location_path) {

	// Looking for exact match
	for (std::size_t i = 0; i < locations.size(); ++i) {
		if (locations[i].path == requested_location_path) {
			return &locations[i];
		}
	}

	// Longest prefix match wins
	const Config::Location*	matched_location = NULL;
	std::size_t matched_location_path_len = 0;

	for (std::size_t i = 0; i < locations.size(); ++i) {

		const Config::Location& config_location = locations[i];
		std::string config_location_path = config_location.path;
		std::size_t config_location_path_len = config_location_path.length();
		std::size_t requested_location_path_len = requested_location_path.length();

		if (startsWith(requested_location_path, config_location_path,
			requested_location_path_len, config_location_path_len)) {

			config_location_path_len = config_location_path.length();
			bool is_valid_boundary =	(config_location_path_len == requested_location_path_len ||
										requested_location_path[config_location_path_len] == '/' ||
										config_location_path == "/");

			if (is_valid_boundary && config_location_path_len > matched_location_path_len) {

				matched_location_path_len = config_location_path_len;
				matched_location = &config_location;

			}
		}
	}

	return matched_location;

}

Dispatcher::content_type_map Dispatcher::initContentTypeMap(void) {

	content_type_map content_types;

	content_types[".html"] = "text/html";
	content_types[".htm"] = "text/html";
	content_types[".css"] = "text/css";
	content_types[".py"] = "text/x-script";
	content_types[".sh"] = "text/x-script";
	content_types[".txt"] = "text/plain";
	content_types[".gif"] = "image/gif";
	content_types[".jpeg"] = "image/jpeg";
	content_types[".jpg"] = "image/jpeg";
	content_types[".png"] = "image/png";
	content_types[".svg"] = "image/svg+xml";
	content_types[".js"] = "application/javascript";
	content_types[".json"] = "application/json";
	content_types[".pdf"] = "application/pdf";
	content_types[".xml"] = "application/xml";

	return content_types;

}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Constructor	*/
Dispatcher::Dispatcher(void) {
	log.debug("Dispatcher Constructor called");
	return;
}

/*	@brief Destructor	*/
Dispatcher::~Dispatcher(void) {
	log.debug("Dispatcher Destructor called");
	return;
}

/*	@brief Copy Constructor	*/
Dispatcher::Dispatcher(const Dispatcher& other) {
	*this = other;
	log.debug("Dispatcher Copy Constructor called");
	return;
}

/*	@brief Copy Assignment Operator	*/
Dispatcher& Dispatcher::operator = (const Dispatcher& other) {
	if (this != &other) {
		log.debug("Dispatcher Copy Assignment Operator called");
	}
	return *this;
}
