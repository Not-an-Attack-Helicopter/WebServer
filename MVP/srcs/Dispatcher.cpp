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
#include "../incs/HTTPResponse.hpp"
#include "../incs/HTTPRequest.hpp"
#include "../incs/constexpr.hpp"
#include "../incs/templates.hpp"
#include "../incs/Config.hpp"
#include "../incs/Logger.hpp"
// #include "../incs/Parser.hpp"
#include "../incs/utils.hpp"
// #include <ostream>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>	// stat
#include <sys/wait.h>	// waitpid
#include <dirent.h>		// opendir, readdir, closedir
#include <cstddef>
#include <fstream>
#include <vector>

static bool isReadable(const std::string& path) {

	if (!isRegularFile(path)) {
		return false;
	}

	return access(path.c_str(), R_OK) == 0;

}

// static bool isWritable(const std::string& path) {
//
// 	// if (!isRegularFile(path)) { // checking directory!
// 	// 	return false;
// 	// }
//
// 	return access(path.c_str(), W_OK) == 0;
//
// }

static bool hasCGIExtension(const Config::Location& location,
							const std::string& path) {

	size_t dot = path.rfind('.');
	if (dot == std::string::npos || dot == path.size() - 1) {
		return false;
	}

	std::string ext = path.substr(dot);
	if (location.interpreters.count(ext) == 1) {
		return true;
	}

	return false;

}

static std::string findIndexFile(const Config::Location& location,
								 const std::string& path) {

	// if (location->index_files.empty()) {
	// 	return "";
	// }

	for (size_t i = 0; i < location.index_files.size(); ++i) {

		std::string index_file_path = path + location.index_files[i];

		// struct stat sb;
		// if (stat(index_file_path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
		// 	return index_file_path;
		// }
		// if (isRegularFile(index_file_path)) {
		return index_file_path;
		// }
	}

	return "";

}

static std::string extractDomainName(const std::string& host_header) {

	size_t colonPos = host_header.find(':');
	if (colonPos != std::string::npos) {
		return host_header.substr(0, colonPos); // Strip port
	}
	return host_header; // No port present

}

// static const bool matchDomain(const std::vector<Config::Domain>& domains,
							  // const std::string& host_header,
							  // Config::Domain matched_domain) {
static const Config::Domain* matchDomain(const std::vector<Config::Domain>& domains,
										 const std::string& host_header) {

	// Return 400 Bad Request if no domain name provided
	if (host_header.empty()) {
		log.error("dispatch error: empty header");
		return NULL;
		// return false;
	}
	std::string requested_domain_name = extractDomainName(host_header);
	// Looking for exact match
	for (size_t i = 0; i < domains.size(); ++i) {
		for (size_t j = 0; j < domains[i].names.size(); ++j) {
			// log.error(requested_domain_name + " == " + domains[i].names[j]);
			if (domains[i].names[j] == requested_domain_name) {
				return &domains[i];
				// matched_domain = domains[i];
				// return true;
			}
		}
	}
	return NULL;
	// return false;
}

// static const Location* matchLocation(const Config* config,
// 									 const std::string& path) {
// 	std::string requested_location = path;
// 	// log.error("requested_location = " + path);
// 	size_t delim = path.find('.');
// 	// log.error(i2a(delim));
// 	if (delim != 0 && delim != std::string::npos) {
// 		delim = path.rfind('/');
// 		// log.error(i2a(delim));
// 		requested_location = path.substr(0, delim + 1);
// 	}
// 	// log.error("requested_location = " + requested_location);
//
// 	const Location*	matched_location = NULL;
// 	// size_t matched_location_path_len = matched_location->path.size();
// 	size_t matched_location_path_len = 0;
//
// 	for (size_t i = 0; i < config->locations.size(); ++i) {
//
// 		const Location* config_location = &config->locations[i];
// 		// std::string config_location_path = config->locations[i].path;
// 		std::string config_location_path = config_location->path;
// 		// log.error("config_location = " + config_location_path);
// 		// log.error(config_location_path + " " + requested_location);
//
// 		size_t j = 0;
// 		while (config_location_path[j] != '\0' && requested_location[j] != '\0') {
//
// 			if (config_location_path[j] != requested_location[j]) {
//
// 				break;
//
// 			} else {
//
// 				++j;
// 				log.error("character match at pos " + i2a(j));
// 				if (j > matched_location_path_len) {
// 					++matched_location_path_len;
// 					matched_location = config_location;
// 					// matched_location = &config->locations[i];
// 					log.error("matched_location = " + matched_location->path);
// 				}
// 			}
// 			// log.error(i2a(j));
// 		}
// 	}
// 	// if (matched_location != NULL)
// 		// log.error("HERE > " + matched_location->path + " < HERE");
// 	return matched_location;
// }

static Method matchMethod(const std::vector<Method>& config_methods,
						  const Method& requested_method) {

	// std::string method = "";
	// const std::string requested_method = request->getMethodName();
	// for (size_t i = 0; i < location->methods.size(); ++i) {
	// 	if (requested_method == location->methods[i]) {
	// 		// method = location.methods[i];
	// 		// method = requested_method;
	// 		// break;
	// 		return requested_method;
	// 	}
	// }
	// return "";
	for (size_t i = 0; i < config_methods.size(); ++i) {

		// log.error("comparing: " + i2a(requested_method) + " against " + i2a(config_methods[i]));
		if (requested_method == config_methods[i]) {
			// matched_method = requested_method;
			// return true;
			// log.error("method found");
			return requested_method;
		}
	}
	// return false;
	log.error("dispatch error: no method found");
	return METHOD_COUNT;
}

static std::string matchContentType(const std::string& path) {

	static const Dispatcher::content_type_map content_types = Dispatcher::initContentTypeMap();
	size_t dot = path.rfind('.');
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

// 	if (ext == ".html" || ext == ".htm")		return "text/html";
// 	else if (ext == ".css")						return "text/css";
// 	else if (ext == ".py" || ext == ".sh")		return "text/x-script";
// 	else if (ext == ".txt")						return "text/plain";
// 	else if (ext == ".gif")						return "image/gif";
// 	else if (ext == ".jpeg" || ext == ".jpg")	return "image/jpeg";
// 	else if (ext == ".png")						return "image/png";
// 	else if (ext == ".svg")						return "image/svg+xml";
// 	else if (ext == ".js")						return "application/javascript";
// 	else if (ext == ".json")					return "application/json";
// 	else if (ext == ".pdf")						return "application/pdf";
// 	else if (ext == ".xml")						return "application/xml";
// 	else										return "application/octet-stream";

static void serveErrorPage(const Config::Location* location,
						   HTTPResponse& response,
						   const StatusCode& code) {

	// const std::map<int, std::string>* location_error_pages = &location->error_pages;
	// const std::map<int, std::string>* server_error_pages = &config.error_pages;
	std::string error_page_path;
	// Check location error_page first, then server error_page
	if (location != NULL && !location->error_pages.empty()) {
		std::map<int, std::string>::const_iterator it = location->error_pages.find(code);
		if (it != location->error_pages.end()) {
			error_page_path = it->second;
		}
	}

	// if (domain != NULL && !domain->error_pages.empty() && error_page_path.empty()) {
	// 	std::map<int, std::string>::const_iterator it = domain->error_pages.find(code);
	// 	if (it != domain->error_pages.end()) {
	// 		error_page_path = it->second;
	// 	}
	// }

	response.setStatus(code);
	if (code == BAD_REQUEST ||
		code == REQUEST_TIMEOUT ||
		code == PAYLOAD_TOO_LARGE ||
		code >= INTERNAL_SERVER_ERROR) {
		response.setHeader("Connection", "close");
	} else {
		response.setHeader("Connection", "keep-alive");
	}

	if (!error_page_path.empty()) {

		// std::ifstream file(error_page_path.c_str());
		// if (file.good()) {
		// 	std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		// 	response->setBodySink(HTTPResponse::TEXT);

		response.setBodySink(DISK);
		response.setBody(error_page_path, "text/html");
		// TEST static_cast<void>((*response).setBody(body, "text/html")); // TEST
		return;

	} else {

		// body = "<html><body><h1>" + response.getStatusReason() + "</h1></body></html>";
		std::ostringstream body ;
		body	<< tag::HTML << tag::BODY << tag::H1 << "Error" << http::_ << i2a(code) << ":"
				<< http::_ << response.getStatusReason() << tag::_H1 << tag::_BODY << tag::_HTML;
		response.setBodySink(HEAP);
		response.setBody(body.str(), "text/html");
		return;

	}

}

static StatusCode serveDirectoryListing(const std::string& path,
										const HTTPRequest& request,
										HTTPResponse& response) {

	// log.error(path);
	DIR* dir = opendir(path.c_str());

	if (!dir) {
		// serveErrorPage(&location, response, FORBIDDEN);
		return FORBIDDEN;
	}

	// std::string body = "<link rel=\"stylesheet\" href=\"../css/style.css\">";
	// std::string body = "<html><body><h1>Index of " + path + "</h1><ul>";
	// std::string body = "<html><body><h1>Index of " + request->getPath() + "</h1><ul>";

	std::ostringstream body;
	body	<< tag::DOC << tag::HTML << tag::HEAD << define::META << define::FAVICON << define::STYLE
			<< tag::TITLE << "Index of" << http::_ << request.getPath() << tag::_TITLE
			<< tag::_HEAD << tag::BODY
			<< tag::H1 << "Index of" << http::_ << request.getPath() << tag::_H1
			<< tag::UL;

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name == ".") continue;
		body	<< tag::LI << tag::A << tag::HREF << name
				<< tag::_HREF << name << tag::_A << tag::_LI << tag::BR;
		// "<li><a href=\"" + name + "\">" + name + "</a></li>"
	}

	body << tag::_UL << tag::_BODY << tag::_HTML;
	// body += "</ul></body></html>";

	closedir(dir);

	response.setStatus(OK);
	response.setHeader("Connection", "keep-alive");
	response.setBodySink(HEAP);
	response.setBody(body.str(), "text/html");

	return OK;

}

static StatusCode handleRedirect(const std::string& path,
								 HTTPResponse& response) {

	std::string new_path = path + "/";

	response.setStatus(MOVED_PERMANENTLY);
	response.setHeader("Connection", "keep-alive");
	response.setHeader("location", new_path);

	std::ostringstream body;
	body << "Moved Permanently. Redirecting to " + new_path;
	response.setBodySink(HEAP);
	response.setBody(body.str(), "text/plain");
	// response.setBody(" ", "text/plain");

	return MOVED_PERMANENTLY;

}

static StatusCode handleRedirect(const Config::Location& location,
								 HTTPResponse& response) {

	// unsigned short max_redirections = 5;
	// unsigned short redirection_count = 0;
	// Location* redirection = const_cast<Location*>(location);
	// while (!redirection->redirect.empty()) {
	// 	++redirection_count;
	// 	log.error(i2a(redirection_count) + " REDIRECTION(S) DETECTED");
	// 	if (redirection_count > Parser::MAX_REDIRECTS) {
	// 		log.error("TOO MANY REDIRECTIONS!");
	// 		// return HTTPResponse::LOOP_DETECTED;
	// 		serveErrorPage(config, location, response, HTTPResponse::LOOP_DETECTED);
	// 		return;
	// 	}
	// 	redirection = const_cast<Location*>(Dispatcher::matchLocation(config->locations, redirection->redirect));
	// }
	response.setStatus(MOVED_PERMANENTLY);
	response.setHeader("Connection", "keep-alive");
	response.setHeader("Location", location.redirect);

	std::ostringstream body;
	body << "Moved Permanently. Redirecting to " + location.redirect;
	response.setBodySink(HEAP);
	response.setBody(body.str(), "text/plain");
	// response->setBody(" ", "text/plain");

	return MOVED_PERMANENTLY;

}

static StatusCode serveFile(const std::string& path,
							HTTPResponse& response) {

	// if(request.getMethod() != "GET")
	// 	_applyErrorPage(client, &location, 405);

	// if (!location)
	// 	_applyErrorPage(config, location, response, 404);

	// struct stat sb;
	// std::string file_path = location.root + "/" + request.getPath();

	// if (stat(file_path.c_str(), &sb) == -1) {
	// 	_applyErrorPage(client, &location, 404);
	// }

	// if (S_ISDIR(sb.st_mode)) {

	if (!isReadable(path)) {
		// log.error("not readable");
		// serveErrorPage(&location, response, FORBIDDEN);
		return FORBIDDEN;
	}
	// std::ifstream file(path.c_str(), std::ios::binary);
	std::ifstream file;
	file.open(path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		// serveErrorPage(&location, response, FORBIDDEN);
		return FORBIDDEN;
	}

	// std::string body((std::istreambuf_iterator<char>(file)),
	// 				 std::istreambuf_iterator<char>());

	// file.seekg(0, std::ios::end);
	// std::streamsize content_length = file.tellg();
	// log.error(i2a(content_length));
	// file.seekg(0, std::ios::beg);  // reset to start // necessary?

	file.close();

	std::string content_type = matchContentType(path);

	// log.error("success");
	response.setStatus(OK);
	response.setHeader("Connection", "keep-alive");
	// response.setBody(body, matchContentType(path));
	// response.setBody(body, content_type);
	response.setBodySink(DISK);
	response.setBody(path, content_type);

	return OK;

}

static StatusCode removeFile(const std::string& path,
							 HTTPResponse& response) {

	// // if (request.getMethod() != "DELETE") {
	// // 	_applyErrorPage(client, &location, 405);
	// // }

	// // if (!location) {
	// // 	_applyErrorPage(config, location, response, 404);
	// // }

	// const HTTPRequest* request = &client.getCurrentRequest();
	// HTTPResponse* response = &client.getCurrentResponse();

	// // struct stat sb;
	// std::string file_path = location->root + "/" + request->getPath();

	// // if (stat(file_path.c_str(), &sb) == -1) {
	// // 	_applyErrorPage(client, &location, 404);
	// // }

	// // if (!S_ISREG(sb.st_mode)) {
	// // 	_applyErrorPage(client, &location, 403);
	// // }

	// if (!isRegularFile(path) && !isDirectory(path)) {
	// 	// serveErrorPage(&location, response, NOT_FOUND);
	// 	return NOT_FOUND;
	// }

	// if (unlink(path.c_str()) == -1) {
	if (std::remove(path.c_str()) != 0) {
		// serveErrorPage(&location, response, FORBIDDEN);
		return INTERNAL_SERVER_ERROR;
	}
	log.info("Deleted " + path);

	response.setStatus(NO_CONTENT);
	response.setHeader("Connection", "keep-alive");

	return NO_CONTENT;

}

static StatusCode handleGet(const Config::Location& location,
							const HTTPRequest& request,
							HTTPResponse& response) {

	std::string path;
	if (!location.root.empty()) path = location.root + request.getPath();
	else path = location.alias + request.getPath().substr(location.path.size());
	// } else if (!location.alias.empty()) {
	// 	path = location.alias;
	// }
	// log.error(path);

	// Only redirect GET requests missing trailing slash
	// (browsers need it for relative links)
	if (path[path.size() - 1] != '/') {
		return handleRedirect(request.getPath(), response);
	}

	// Check if index file present
	// log.error(path);
	std::string index_file_path = findIndexFile(location, path);
	// log.error(index_file_path);
	// Return index file
	if (!index_file_path.empty() && isReadable(index_file_path)) {
		return serveFile(index_file_path, response); // TEST
	// No index file found, check if autoindex is enabled
	// autoindex is on, generate directory listing
	} else if (location.autoindex) {
		// log.error(path);
		return serveDirectoryListing(path, request, response); // TEST
	// autoindex is off, return 403
	} else {
		// serveErrorPage(&location, response, FORBIDDEN);
		return FORBIDDEN;
	}

}

static StatusCode handlePost(const Config::Location& location,
							 const HTTPRequest& request,
							 HTTPResponse& response) {

	// // if (request.getMethod() != "POST") {
	// // 	_applyErrorPage(client, &location, 405);
	// // }

	// // if (!location) {
	// // 	_applyErrorPage(client, &location, 404);
	// // }

	// const HTTPRequest* request = &client.getCurrentRequest();
	// HTTPResponse* response = &client.getCurrentResponse();

	// if (request.getBody().size() > client_max_body_size) {
	// 	// response.setStatus(PAYLOAD_TOO_LARGE);
	// 	return PAYLOAD_TOO_LARGE;
	// }

	// struct stat sb;

	// // if (stat(upload_dir.c_str(), &sb) == -1 || !S_ISDIR(sb.st_mode)) {
	// // 	_applyErrorPage(client, &location, 403);
	// // }
	// if (!isWritable(upload_dir)) {
	// 	// serveErrorPage(&location, response, FORBIDDEN);
	// 	return FORBIDDEN;
	// }

	if (request.getPath().find("..") != std::string::npos) {
		return BAD_REQUEST;
	}

	if (request.getBody().empty()) {
		// serveErrorPage(&location, response, BAD_REQUEST);
		return BAD_REQUEST;
	}

	std::string path;
	if (!location.root.empty()) path = location.root + request.getPath();
	else path = location.alias + request.getPath().substr(location.path.size());
	// log.error(path);

	std::string out_path = location.upload_dir + "/upload.bin";
	std::ofstream out(out_path.c_str(), std::ios::binary);

	if (!out.is_open()) {
		// serveErrorPage(&location, response, INTERNAL_SERVER_ERROR);
		return INTERNAL_SERVER_ERROR;
	}

	out << request.getBody();
	out.close();

	response.setStatus(CREATED);
	response.setHeader("Connection", "keep-alive");

	std::string body = "Uploaded\n";
	response.setBody(body, "text/plain");

	return CREATED;

}

static StatusCode handleDirectory(const Method& method,
								  const Config::Location& location,
								  const HTTPRequest& request,
								  HTTPResponse& response) {

	switch (method) {
	case GET:
		return handleGet(location, request, response);
		break;
	case POST:
		return handlePost(location, request, response); // TEST // TODO //
		break;
	case DELETE:
		return METHOD_NOT_ALLOWED;
		break;
	default:
		return NOT_IMPLEMENTED;
	}

}

static StatusCode handleRegularFile(const Method& method,
									const std::string& path,
									HTTPResponse& response) {

	switch (method) {
	case GET:
		return serveFile(path, response); // TEST
		break;
	case POST:
		return METHOD_NOT_ALLOWED;
		break;
	case DELETE:
		return removeFile(path, response); // TEST // TODO //
		break;
	default:
		return NOT_IMPLEMENTED;
	}

}

// static StatusCode handleDirectory(const Method& method,
// 								  const Config::Location& location,
// 								  const HTTPRequest& request,
// 								  HTTPResponse& response) {
// 	// Only redirect GET requests missing trailing slash
// 	// (browsers need it for relative links)
// 	if (path[path.size() - 1] != '/') {
// 		return handleRedirect(request.getPath(), response);
// 		// return;
// 	}
//
// 	switch (method) {
// 	// if (method == "GET") {
// 	case GET:
// 		return handleGet(location, request, response, path);
// 		// return; // TODO
// 	// } else if (method == "POST") {
// 	case POST:
// 		return handlePost(location, request, response); // TEST
// 		// return; // TODO
// 	// case DELETE:
// 	// 	return handleDelete(response, path); // TEST
// 	// } else {
// 		// return; // TODO
// 	default:
// 		return METHOD_NOT_ALLOWED;;
// 	}
//
// }

// static unsigned short handleRegularFile(const Config* config,
// 										const Location* location,
// 										const HTTPRequest* request,
// 										HTTPResponse* response,
// 										const std::string& path,
// 										const std::string& method) {
//
// 	if (method == "GET") {
// 		handleGet(config, location, request, response, path);
// 		return 0;
// 	} else if (method == "POST") {
// 		handlePost(config, location, request, response); // TEST
// 		return 0;
// 	} else {
// 		handleDelete(config, location, response, path); // TEST
// 		return 0;
// 	}
// 	return 0;
//
// }

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

void Dispatcher::handleRequest(Client& client) {

	StatusCode status_code;

	const Config::Socket& socket = client.getConfig();
	const HTTPRequest& request = client.getCurrentRequest();
	HTTPResponse& response = client.getCurrentResponse();

	// Check if valid request received
	if (request.parsing.state == HTTPRequest::ERROR) {
		client.keepAlive = false;
		return serveErrorPage(NULL, response, BAD_REQUEST);
	}

	// Check HTTP version
	std::string version = request.getVersion();
	if (version == http::V_1_1) {
		if (request.hasHeader("connection") && request.getHeader("connection") == "close") {
			response.setHeader("Connection", "close");
			client.keepAlive = false;
		}
	} else if (version ==  http::V_1_0) {
		if (!request.hasHeader("connection") || request.getHeader("connection") !=  "keep-alive") {
			response.setHeader("Connection", "close");
			client.keepAlive = false;
		}
	}

	// Match domain by name
	// if (!matchDomain(socket->domains, request->getHeader("host"), domain)) {
	const Config::Domain* dom_ptr = matchDomain(socket.domains, request.getHeader("host"));
	if (!dom_ptr) {
		client.keepAlive = false;
		return serveErrorPage(NULL, response, BAD_REQUEST); // Bad Request
	}
	const Config::Domain& domain = *dom_ptr;

	// Match location by path
	const Config::Location* loc_ptr = Dispatcher::matchLocation(domain.locations, request.getPath());
	if (!loc_ptr) {
		// log.error("NO LOCATION FOUND");
		return serveErrorPage(NULL, response, NOT_FOUND); // Not Found
	}
	const Config::Location& location = *loc_ptr;
	// log.error(location->path);

	// Check if return directive exists
	if (!location.redirect.empty()) {
		status_code = handleRedirect(location, response);
		// if (status_code < BAD_REQUEST) {
		// 	return;
		// }
		return;
	}

	// Check if method allowed
	// const std::string method = matchMethod(location, request);
	// if (method.empty()) {
	// 	serveErrorPage(config, location, response, HTTPResponse::METHOD_NOT_ALLOWED); // Method Not Allowed
	// 	return;
	// }

	// log.error(i2a(client.getConfig().domains[0].locations[0].methods[0]));
	// log.error(i2a(socket.domains[0].locations[0].methods[0]));
	// log.error(i2a(domain.locations[0].methods[0]));
	// log.error(i2a(location.methods[0]));
	Method method = matchMethod(location.methods, request.getMethod());
	if (method == METHOD_COUNT) {
		return serveErrorPage(loc_ptr, response, METHOD_NOT_ALLOWED); // Method Not Allowed
	}
	// Create absolute path from root
	// std::string path;
	// if (location->root.empty()) {
	// 	path = config->root + "/" + request->getPath();
	// } else {
	// 	path = location->root + "/" + request->getPath();
	// }
	// location->root.empty() ?
	// path = config->root + "/" + request->getPath() :
	std::string path;
	if (!location.root.empty()) path = location.root + request.getPath();
	else path = location.alias + request.getPath().substr(location.path.size());
	// log.error(path);

	// Match CGI extensions
	if (hasCGIExtension(location, path)) {
		// status_code = executeCGI(location, path, request, response); // TODO
		// if (status_code < BAD_REQUEST) {
		// 	return;
		// }

	// Check if request is for a directory
	} else if (isDirectory(path)) {

		// log.debug("Directory detected");

		// switch (method) {
		// case GET:
		// 	status_code = handleGet(location, request, response);
		// 	break;
		// case DELETE:
		// 	return serveErrorPage(loc_ptr, response, METHOD_NOT_ALLOWED);
		// 	break;
		// case POST:
		// 	if (method == POST && request.getBody().size() > domain.client_max_body_size) {
		// 		return serveErrorPage(loc_ptr, response, PAYLOAD_TOO_LARGE);
		// 	}
		// 	status_code = handlePost(location, request, response); // TEST
		// 	break;
		// default:
		// 	return serveErrorPage(loc_ptr, response, NOT_IMPLEMENTED);
		// }

		if (method == POST && request.getBody().size() > domain.client_max_body_size) {
			client.keepAlive = false;
			return serveErrorPage(loc_ptr, response, PAYLOAD_TOO_LARGE);
		// } else if (method == POST && request.getBody().empty()) {
		// 	response.setHeader("Connection", "close");
		// 	client.closeAfterSend = true;
		// 	return serveErrorPage(loc_ptr, response, BAD_REQUEST);
		}
		status_code = handleDirectory(method, location, request, response);
		if (status_code < BAD_REQUEST) {
			return;
		}
		// status_code = handleDirectory(location, request, response, path, method);
		// status_code = handleDirectory(location, request, response, path, method, domain.client_max_body_size);
		// return;

	// Check if request path exists as static file in `root`
	} else if (isRegularFile(path)) {

		// log.debug("Regular file detected");

		// std::string content_type;
		// static const content_type_map content_types = initContentTypeMap();
		// std::string ext;
		// size_t dot = path.rfind('.');
		// if (dot == std::string::npos || dot == path.size() - 1) {
		// 	content_type = "application/octet-stream";
		// } else {
		// 	ext = path.substr(dot);
		// 	if (content_types.find(ext) != content_types.end()) {
		// 		content_type = content_types.at(ext);
		// 	} else {
		// 		content_type = "application/octet-stream";
		// 	}
		// }

		// switch (method) {
		// case POST:
		// 	return serveErrorPage(loc_ptr, response, METHOD_NOT_ALLOWED);
		// 	break;
		// case GET:
		// 	status_code = serveFile(path, response); // TEST
		// 	break;
		// case DELETE:
		// 	// TODO //
		// 	status_code = removeFile(path, response); // TEST
		// 	// TODO //
		// 	break;
		// default:
		// 	return serveErrorPage(loc_ptr, response, NOT_IMPLEMENTED);
		// }

		status_code = handleRegularFile(method, path, response);
		if (status_code < BAD_REQUEST) {
			return;
		}

	// If error occurred, check `error_page` directive
	} else {
		// int code = 500;
		// serveErrorPage(config, location, response, code);
		// return;
		log.error("REACHED END OF BRANCHES");
		status_code = NOT_FOUND;

	}

	// TODO //
	// if (status >= Bad) {
	// 	client.closeAfterSend = true;
	// }
	// if (status_code == BAD_REQUEST ||
	// 	status_code == REQUEST_TIMEOUT ||
	// 	status_code == PAYLOAD_TOO_LARGE ||
	// 	status_code >= INTERNAL_SERVER_ERROR) {
	// 	response.setHeader("Connection", "close");
	// 	client.closeAfterSend = true;
	// }
	// TODO //

	// if (status_code >= BAD_REQUEST) {
	log.error("STATUS CODE: " + i2a(status_code));
	serveErrorPage(&location, response, status_code);
	return;
	// }

}

// TODO // DECISION REQUIRED // TODO
// if (request->getPath() == location->upload_dir) {
// 	_handleUpload(client, location);
// }

// if(response->getBody().empty()) {
// 	response->setBody("", "text/plain");
// 	return;
// }

static bool startsWith(const std::string& requested_path,
					   const std::string& config_location_path,
					   size_t requested_location_path_len,
					   size_t config_location_path_len) {

	if (config_location_path_len > requested_location_path_len) {
		return false;
	}

	return requested_path.compare(0, config_location_path_len, config_location_path) == 0;

}

const Config::Location* Dispatcher::matchLocation(const std::vector<Config::Location>& locations,
												  const std::string& requested_location_path) {

	// Looking for exact match
	for (size_t i = 0; i < locations.size(); ++i) {
		if (locations[i].path == requested_location_path) {
			return &locations[i];
		}
	}

	// Longest prefix match wins
	const Config::Location*	matched_location = NULL;
	size_t matched_location_path_len = 0;

	for (size_t i = 0; i < locations.size(); ++i) {

		const Config::Location& config_location = locations[i];
		std::string config_location_path = config_location.path;
		size_t config_location_path_len = config_location_path.length();
		size_t requested_location_path_len = requested_location_path.length();
		// log.error(config_location_path + " " + i2a(config_location_path_len));
		// log.error(requested_location_path + " " + i2a(requested_location_path_len));

		// if (requested_location_path_len <= config_location_path_len &&
		// 	requested_location_path.compare(0, config_location_path_len, config_location_path) == 0) {
		if (startsWith(requested_location_path, config_location_path,
			requested_location_path_len, config_location_path_len)) {

			// log.error("A config location matches with requested location.");
			config_location_path_len = config_location_path.length();
			bool is_valid_boundary =	(config_location_path_len == requested_location_path_len ||
										requested_location_path[config_location_path_len] == '/' ||
										config_location_path == "/");
			// log.error(std::string("") + (requested_location_path[config_location_path_len]));
			// log.error(is_valid_boundary ? "valid boundary" : "invalid boundary");
			// log.error(i2a(config_location_path_len) + " vs " + i2a(matched_location_path_len));

			if (is_valid_boundary && config_location_path_len > matched_location_path_len) {

				matched_location_path_len = config_location_path_len;
				matched_location = &config_location;
				// log.error("New match found! " + matched_location->path + " " + i2a(config_location_path_len));

			}
		}
	}

	// if (matched_location != NULL)
	// 	log.error("HERE > " + matched_location->path + " < HERE");
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

/*	@brief Deconstructor	*/
Dispatcher::~Dispatcher(void) {
	log.debug("Dispatcher Deconstructor called");
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

	// 	size_t dot = path.rfind('.');
	// 	if (dot == std::string::npos || dot == path.size() - 1)
	// 		return "application/octet-stream";

	// 	std::string ext = path.substr(dot);

	// 	if (ext == ".html" || ext == ".htm")		return "text/html";
	// 	else if (ext == ".css")						return "text/css";
	// 	else										return "application/octet-stream";

// void Dispatcher::_applyErrorPage(const Config& config,
// 									HTTPResponse& response,
// 									int code) {
//
// 	// Check location error_page first, then server error_page
// 	std::map<int, std::string>::const_iterator it = config.error_pages.find(code);
// 	// _matchStatusCode(code);
//
// 	std::string body;
// 	if (it != config.error_pages.end()) {
// 		std::string page_path = it->second;
//
// 		if (!page_path.empty() && page_path[0] != '/')
// 			page_path = config.root + "/" + page_path;
//
// 		std::ifstream file(page_path.c_str(), std::ios::binary);
// 		if (file.is_open()) {
// 			body.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
// 			file.close();
// 			response.setBody(body, _matchContentType(it->second));
// 		}
//
// 	}
//
// 	// body = "<html><body><h1>" + response.getStatusReason() + "</h1></body></html>";
// 	body =	"<html><body><h1>" + i2a(response.getStatusCode()) +
// 			": " + response.getStatusReason() + "</h1></body></html>";
//
// 	response.setStatus(code);
// 	response.setHeader("Server", "MyServer/1.0");
// 	response.setBody(body, "text/html");
//
// 	return;

// }

// if (config_location.path.size() < requested_location.size()) {
// 	if (!config_location.path.compare(0, config_location.path.size(), requested_location)) {
// 		if (config_location.path.size() > path_len) {
// 			matched_location = &config->locations[i];
// 			path_len = config_location.path.size();
// 		}
// 	}
// } else {
// 	if (!requested_location.compare(0, requested_location.size(), config_location.path)) {
// 		if (config_location.path.size() > path_len) {
// 			matched_location = &config->locations[i];
// 			path_len = config_location.path.size();
// 		}
// 	}
// }

// std::regex pattern(std::string("^" + config_location.path + "/?$"));
// try {
// 	if (regex_match(requested_location, pattern)) {
// 		matched_location = &config->locations[i];
// 		path_len = config_location.path.size();
// 	}
// } catch (const std::regex_error& e) {
// 	throw std::runtime_error("regex error: " + std::string(e.what()));
// }


// // Now dispatch based on method and location config
// if (method == "GET") {
// 	_handleGet(location, request, response);
// } else if (method == "POST") {
// 	_handlePost(location, request, response);
// } else if (method == "DELETE") {
// 	_handleDelete(location, request, response);
// }

// for (size_t i = 0; i < location.error_pages.size(); ++i) {
// 	if (location.error_pages[i].first == code) {
// 		errorPagePath = location.error_pages[i].second;
// 		break;
// 	}
// }

// for (size_t i = 0; i < config.error_pages.size(); ++i) {
// 	if (config.error_pages[i].first == code) {
// 		errorPagePath = config.error_pages[i].second;
// 		break;
// 	}
// }

// if (!request->getQuery().empty()) {
// 	_handleCGI(config, location, method, request, response);
// 	_executeCGI(location, request, response);

// 4-6. Check directory, autoindex, index
// // Check if request is for a directory
// 	if (isDirectory(req.getPath())) {
// 		if (!loc->getAutoindex()) {
// 			res.setStatus(403);
// 			applyErrorPage(res, 403, loc);
// 			return;
// 		}
//
// 		std::string indexFile = findIndexFile(req.getPath(), loc);
// 		if (!indexFile.empty()) {
// 			serveFile(indexFile, res);
// 			return;
// 		}
//
// 		// No index file found
// 		// autoindex is on, serve directory listing
// 		serveDirectoryListing(req.getPath(), res);
// 		return;
// 	}
// 	Check `autoindex` directive
// 	Check if directory has matching `index` file
// 	} else if (location.autoindex) {
// 		_handleAutoindex(config, request, response, location.root + request.getPath());
