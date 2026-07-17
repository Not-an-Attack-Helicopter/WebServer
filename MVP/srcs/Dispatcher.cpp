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
#include "../incs/templates.hpp"
#include "../incs/Config.hpp"
#include "../incs/Logger.hpp"
#include "../incs/utils.hpp"
#include <sys/stat.h>	// stat
#include <sys/wait.h>	// waitpid
#include <dirent.h>		// opendir, readdir, closedir
#include <cstddef>
#include <fstream>

static bool isReadable(const std::string& path) {

	if (!isRegularFile(path)) {
		return false;
	}

	return access(path.c_str(), R_OK) == 0;

}

static bool isWritable(const std::string& path) {

	// if (!isRegularFile(path)) { // Checking directory!
	// 	return false;
	// }

	return access(path.c_str(), W_OK) == 0;

}

static bool hasCGIExtension(const Location* location,
							const std::string& path) {

	size_t dot = path.rfind('.');
	if (dot == std::string::npos || dot == path.size() - 1) {
		return false;
	}

	std::string ext = path.substr(dot);
	if (location->interpreters.count(ext) == 1) {
		return true;
	}

	return false;

}

static std::string findIndexFile(const Location* location,
								 const std::string& path) {

	// if (location->index_files.empty()) {
	// 	return "";
	// }

	for (size_t i = 0; i < location->index_files.size(); ++i) {

		std::string index_file_path = path + location->index_files[i];

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

static bool startsWith(const std::string& requested_path,
					   const std::string& config_location_path,
					   size_t requested_location_path_len,
					   size_t config_location_path_len) {

	if (config_location_path_len > requested_location_path_len) {
		return false;
	}

	return requested_path.compare(0, config_location_path_len, config_location_path) == 0;

}

static const Location* matchLocation(const Config* config,
									 const std::string& requested_location_path) {

	// Look for exact match
	for (size_t i = 0; i < config->locations.size(); ++i) {
		if (requested_location_path == config->locations[i].path) {
			return &config->locations[i];
		}
	}

	// Longest prefix match wins
	const Location*	matched_location = NULL;
	size_t matched_location_path_len = 0;

	for (size_t i = 0; i < config->locations.size(); ++i) {

		const Location* config_location = &config->locations[i];
		std::string config_location_path = config_location->path;
		size_t config_location_path_len = config_location_path.length();
		size_t requested_location_path_len = requested_location_path.length();
		// log.error(config_location_path + " " + i2a(config_location_path_len));
		// log.error(requested_location_path + " " + i2a(requested_location_path_len));
		if (startsWith(requested_location_path,
					   config_location_path,
					   requested_location_path_len,
					   config_location_path_len)) {

			// log.error("A config location matches with requested location.");
			config_location_path_len = config_location_path.length();
			bool is_valid_boundary = 	(config_location_path_len == requested_location_path_len ||
										requested_location_path[config_location_path_len] == '/' ||
										config_location_path == "/");
			// log.error(is_valid_boundary ? "valid boundary" : "invalid boundary");
			// log.error(i2a(config_location_path_len) + " vs " + i2a(matched_location_path_len));

			if (is_valid_boundary && config_location_path_len > matched_location_path_len) {

				matched_location_path_len = config_location_path_len;
				matched_location = config_location;
				// log.error("New match found! " + matched_location->path + " " + i2a(config_location_path_len));

			}

		}

	}

	// if (matched_location != NULL)
	// 	log.error("HERE > " + matched_location->path + " < HERE");
	return matched_location;

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

static std::string matchMethod(const Location* location,
							   const HTTPRequest* request) {

	// std::string method = "";
	std::string requested_method = request->getMethod();

	for (size_t i = 0; i < location->methods.size(); ++i) {

		if (requested_method == location->methods[i]) {
			// method = location.methods[i];
			// method = requested_method;
			// break;
			return requested_method;
		}

	}

	return "";
}

static std::string matchContentType(const std::string& path) {

	size_t dot = path.rfind('.');
	if (dot == std::string::npos || dot == path.size() - 1)
		return "application/octet-stream";

	std::string ext = path.substr(dot);
	if (ext == ".html" || ext == ".htm")		return "text/html";
	else if (ext == ".css")						return "text/css";
	else if (ext == ".gif")						return "image/gif";
	else if (ext == ".js")						return "application/javascript";
	else if (ext == ".json")					return "application/json";
	else if (ext == ".jpeg" || ext == ".jpg")	return "image/jpeg";
	else if (ext == ".pdf")						return "application/pdf";
	else if (ext == ".png")						return "image/png";
	else if (ext == ".py" || ext == ".sh")		return "text/x-script";
	else if (ext == ".svg")						return "image/svg+xml";
	else if (ext == ".txt")						return "text/plain";
	else if (ext == ".xml")						return "application/xml";
	else										return "application/octet-stream";

}

static void serveErrorPage(const Config* config,
						   const Location* location,
						   HTTPResponse* response,
						   int code) {

	// const std::map<int, std::string>* location_error_pages = &location->error_pages;
	// const std::map<int, std::string>* server_error_pages = &config.error_pages;

	// Check location error_page first, then server error_page
	std::string error_page_path;
	if (location != NULL && !location->error_pages.empty()) {

		std::map<int, std::string>::const_iterator it = location->error_pages.find(code);
		if (it != location->error_pages.end()) {
			error_page_path = it->second;
		}

	}

	if (location != NULL && !config->error_pages.empty() && error_page_path.empty()) {

		std::map<int, std::string>::const_iterator it = config->error_pages.find(code);
		if (it != config->error_pages.end()) {
			error_page_path = it->second;
		}

	}

	if (!error_page_path.empty()) {

		response->setStatus(code);
		response->setHeader("Server", "MyServer/1.0");

		std::ifstream file(error_page_path.c_str());
		if (file.good()) {
			std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			response->setBody(body, "text/html");
			// TEST (void)(*response).setBody(body, "text/html"); // TEST
			return;
		}

	} else {

		response->setStatus(code);
		response->setHeader("Server", "MyServer/1.0");

		// body = "<html><body><h1>" + response.getStatusReason() + "</h1></body></html>";
		std::string body =	"<html><body><h1>Error " + i2a(code) + ": "
		+ response->getStatusReason() + "</h1></body></html>";
		response->setBody(body, "text/html");
		return;

	}

}

static void serveFile(const Config* config,
					  const Location* location,
					  HTTPResponse* response,
					  const std::string& path) {

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
		serveErrorPage(config, location, response, 403);
		return;
	}
	// std::ifstream file(path.c_str(), std::ios::binary);
	std::ifstream file(path.c_str());
	if (!file.is_open()) {
		serveErrorPage(config, location, response, 403);
		return;
	}

	std::string body((std::istreambuf_iterator<char>(file)),
					 std::istreambuf_iterator<char>());

	file.close();

	// log.error("success");
	response->setStatus(200, "OK");
	response->setHeader("Server", "MyServer/1.0");
	response->setBody(body, matchContentType(path));

	return;

}

static void serveDirectoryListing(const Config* config,
								  const Location* location,
								  const HTTPRequest* request,
								  HTTPResponse* response,
								  const std::string& path) {

	// log.error(path);
	DIR* dir = opendir(path.c_str());

	if (!dir) {
		serveErrorPage(config, location, response, 403);
		return;
	}

	// std::string body = "<html><link rel=\"stylesheet\" href=\"../css/style.css\"><body><h1>Index of " + path + "</h1><ul>";
	std::string body = "<html><body><h1>Index of " + request->getPath() + "</h1><ul>";

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name == ".") continue;
		body += "<li><a href=\"" + name + "\">" + name + "</a></li>";
	}

	body += "</ul></body></html>";

	closedir(dir);

	response->setStatus(200, "OK");
	response->setHeader("Server", "MyServer/1.0");
	response->setBody(body, "text/html");

	return;

}

static void handleRedirect(const std::string& path,
						   HTTPResponse* response) {

	std::string dir_path = path + "/";

	response->setStatus(301);
	response->setHeader("Server", "MyServer/1.0");
	response->setHeader("location", dir_path);

	std::string body = "Moved Permanently. Redirecting to " + dir_path;
	response->setBody(body, "text/plain");
	// response->setBody(" ", "text/plain");

	return;

}

static void handleRedirect(const Location* location,
						   HTTPResponse* response) {

	response->setStatus(301);
	response->setHeader("Server", "MyServer/1.0");
	response->setHeader("location", location->redirect);

	std::string body = "Moved Permanently. Redirecting to " + location->redirect;
	response->setBody(body, "text/plain");
	// response->setBody(" ", "text/plain");

	return;

}

static void handleGet(const Config* config,
					  const Location* location,
					  const HTTPRequest* request,
					  HTTPResponse* response,
					  const std::string& path) {

	// Check if index file present
	// log.error(path);
	std::string index_file_path = findIndexFile(location, path);
	// log.error(index_file_path);
	// Return index file
	if (!index_file_path.empty() && isReadable(index_file_path)) {

		serveFile(config, location, response, index_file_path); // TEST
		return;

	// No index file found, check if autoindex is enabled
	// autoindex is on, generate directory listing
	} else if (location->autoindex) {

		// log.error(path);
		serveDirectoryListing(config, location, request, response, path); // TEST
		return;

	// autoindex is off, return 403
	} else {

		serveErrorPage(config, location, response, 403);
		return;

	}

}

static void handlePost(const Config* config,
					   const Location* location,
					   const HTTPRequest* request,
					   HTTPResponse* response) {

	// // if (request.getMethod() != "POST") {
	// // 	_applyErrorPage(client, &location, 405);
	// // }

	// // if (!location) {
	// // 	_applyErrorPage(client, &location, 404);
	// // }

	// const HTTPRequest* request = &client.getCurrentRequest();
	// HTTPResponse* response = &client.getCurrentResponse();

	if (request->getBody().empty()) {
		serveErrorPage(config, location, response, 400);
		return;
	}

	if (request->getBody().size() > config->client_max_body_size) {
		response->setStatus(413, "Payload Too Large");
		return;
	}

	// struct stat sb;
	std::string upload_dir = location->upload_dir.empty() ? location->root : location->upload_dir;

	// // if (stat(upload_dir.c_str(), &sb) == -1 || !S_ISDIR(sb.st_mode)) {
	// // 	_applyErrorPage(client, &location, 403);
	// // }
	if (!isWritable(upload_dir)) {
		serveErrorPage(config, location, response, 403);
		return;
	}

	std::string out_path = upload_dir + "/upload.bin";
	std::ofstream out(out_path.c_str(), std::ios::binary);

	if (!out.is_open()) {
		serveErrorPage(config, location, response, 500);
		return;
	}

	out << request->getBody();
	out.close();

	response->setStatus(201, "Created");
	response->setHeader("Server", "MyServer/1.0");
	response->setBody("Uploaded\n", "text/plain");

	return;

}

static void handleDelete(const Config* config,
						 const Location* location,
						 HTTPResponse* response,
						 const std::string& path) {

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
	// log.debug("handleDelete: " + path + "--------------------------------------------------------------------------------------------------------");

	if (!isRegularFile(path) && !isDirectory(path)) {
		serveErrorPage(config, location, response, 404);
		return;
	}
	if (std::remove(path.c_str()) == -1) {
		serveErrorPage(config, location, response, 500);
		return;
	}
	log.debug("handleDelete: deleted " + path);

	response->setStatus(204, "No Content");
	return;

}

static void handleDirectory(const Config* config,
							const Location* location,
							const HTTPRequest* request,
							HTTPResponse* response,
							const std::string& path,
							const std::string& method) {

	// Only redirect GET requests missing trailing slash (browsers need it for relative links)
	if (path[path.size() - 1] != '/') {
		handleRedirect(request->getPath(), response);
		return;
	}

	// log.error(path);
	if (method == "GET") {

		handleGet(config, location, request, response, path);
		return;

	} else if (method == "POST") {

		handlePost(config, location, request, response); // TEST
		return;

	} else {

		handleDelete(config, location, response, path); // TEST
		return;

	}

}

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Instance	*/
Dispatcher& Dispatcher::instance(void) {
	static Dispatcher instance;
	return instance;
}

void Dispatcher::dispatchRequest(Client& client) {

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

	unsigned short status = 0;

	const Config* config = &client.getConfig();
	const HTTPRequest* request = &client.getCurrentRequest();
	HTTPResponse* response = &client.getCurrentResponse();

	// Match location by path
	const Location* location = matchLocation(config, request->getPath());
	if (!location) {
		// log.error("404");
		serveErrorPage(config, location, response, 404); // Not found
		return;
	}

	// Check if return directive exists
	if (!location->redirect.empty()) {
		handleRedirect(location, response);
		return;
	}

	// Check if method allowed
	const std::string method = matchMethod(location, request);
	if (method.empty()) {
		serveErrorPage(config, location, response, 405); // Method Not Allowed
		return;
	}

	// Create absolute path from root
	std::string path;
	// if (location->root.empty()) {
	// 	path = config->root + "/" + request->getPath();
	// } else {
	// 	path = location->root + "/" + request->getPath();
	// }
	// location->root.empty() ?
	// path = config->root + "/" + request->getPath() :
	path = location->root + request->getPath();
	// log.error(path);

	// Match CGI extensions
	if (hasCGIExtension(location, path)) {
		// _executeCGI(location, path, request, response); // TODO
		return;

	// Check if request is for a directory
	} else if (isDirectory(path)) {

		// log.error("directory detected");
		handleDirectory(config, location, request, response, path, method); // TEST
		return;

	// Check if request path exists as static file in `root`
	} else if (isRegularFile(path)) {
		// log.error("regular file detected");
		if (method == "DELETE") {
			handleDelete(config, location, response, path);
		} else if (method == "POST") {
			handlePost(config, location, request, response);
		} else {
			serveFile(config, location, response, path);
		}
		return;

	// If error occurred, check `error_page` directive
	} else {
		// int code = 500;
		// serveErrorPage(config, location, response, code);
		// return;
		status = 500;
	}

	if (status >= 400) {
		log.error("over 400!!");
		serveErrorPage(config, location, response, status);
		return;
	}
	// if(response->getBody().empty()) {
	// 	response->setBody("", "text/plain");
	// 	return;
	// }

}

// TODO // DECISION REQUIRED // TODO
// if (request->getPath() == location->upload_dir) {
// 	_handleUpload(client, location);
// }

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
	if (this != &other) {}
	log.debug("Dispatcher Copy Constructor called");
	return;
}

/*	@brief Copy Assignment Operator	*/
Dispatcher& Dispatcher::operator = (const Dispatcher& other) {
	log.debug("Dispatcher Copy Assignment Operator called");
	if (this != &other) {}
	return *this;
}

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
