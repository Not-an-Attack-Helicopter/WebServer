#include "../incs/RequestHandler.hpp"
#include "../incs/utils.hpp"
#include "../incs/types.hpp"
#include <sys/stat.h>   // stat
#include <sys/wait.h>   // waitpid
#include <dirent.h>     // opendir, readdir, closedir
#include <fstream>
#include <algorithm>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Constructor	*/
RequestHandler::RequestHandler(void) {
	return;
}


/*	@brief Destructor	*/
RequestHandler::~RequestHandler(void) {
	return;
}

/*	@brief Copy Constructor	*/
RequestHandler::RequestHandler(const RequestHandler& other) {
	return;
}

/*	@brief Copy Assignment Operator	*/
RequestHandler& RequestHandler::operator = (const RequestHandler& other) {
	if (this != &other) {
		_location = other._location;
	}
	return *this;
}

void RequestHandler::handle(HTTPRequest* request, HTTPResponse* response, Config* config) {

	if (_location && !_location->redirect.empty())
		_handleRedirect(request, response);
	else if (_location && !_location->cgi_extension.empty())
		_handleStatic(request, response);
	else if (_location && _location->autoindex)
		_handleAutoindex(response, _location->root + _request->getPath());
	else if (_location && !_location->upload_dir.empty())
		_handleUpload(request, response);
	else if (_location && std::find(_location->methods.begin(), _location->methods.end(), "DELETE") != _location->methods.end())
		_handleDelete(request, response);
	else
		_handleStatic(request, response);

}

void RequestHandler::handle(const HTTPRequest& req, HTTPResponse& res, const Config& config) {
	std::string reqPath = req.getPath();
	std::string reqMethod = req.getMethod();

	// Find matching LocationConfig
	LocationConfig* matchedLocation = NULL;
	for (size_t i = 0; i < config.locations.size(); ++i) {
	// if (matchedPath(config.locations[i].path, reqPath)) {
	//   matchedLocation = &config.locations[i];
	//   break; // Or implement priority logic if needed
	// }
	}

	if (!matchedLocation) {
		res.setStatus(404);
	return;
	}

	// Check if method is allowed
	bool methodAllowed = false;
	for (size_t i = 0; i < matchedLocation->methods.size(); ++i) {
		if (matchedLocation->methods[i] == reqMethod) {
			methodAllowed = true;
			break;
		}
	}

	if (!methodAllowed) {
		res.setStatus(405); // Method Not Allowed
		return;
	}

	// Now dispatch based on method and location config
	// if (reqMethod == "GET") {
	// 	handleGet(req, res, *matchedLocation);
	// } else if (reqMethod == "POST") {
	// 	handlePost(req, res, *matchedLocation);
	// } else if (reqMethod == "DELETE") {
	// 	handleDelete(req, res, *matchedLocation);
	// }
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

const LocationConfig* RequestHandler::_matchLocation(void) const {

	const LocationConfig* best = NULL;
	size_t best_len = 0;
	const std::string& path = _request->getPath();

	for (size_t i = 0; i < _config->locations.size(); ++i)
	{
		const LocationConfig& loc = _config->locations[i];
		if (path.compare(0, loc.path.size(), loc.path) == 0)
		{
			if (loc.path.size() > best_len)
			{
				best = &_config->locations[i];
				best_len = loc.path.size();
			}
		}
	}
	return best;

}

void RequestHandler::_handleRedirect(HTTPRequest* request, HTTPResponse* response) {

	response->setStatus(301, "Moved Permanently");
	response->setHeader("Location", _location->redirect);

}

void RequestHandler::_handleStatic(HTTPRequest* request, HTTPResponse* response) {

	if(_request->getMethod() != "GET")
		_errorResponse(response, 405);

	if (!_location)
		_errorResponse(response, 404);

	struct stat st;
	std::string file_path = _location->root + _request->getPath();

	if (stat(file_path.c_str(), &st) == -1)
		_errorResponse(response, 404);

	if (S_ISDIR(st.st_mode)) {

		if (_location->index.empty())
			_handleAutoindex(response, file_path);
		file_path += "/" + _location->index;

		if (stat(file_path.c_str(), &st) == -1 || !S_ISREG(st.st_mode))
			_errorResponse(response, 404);

	}

	std::ifstream in(file_path.c_str(), std::ios::binary);

	if (!in.is_open())
		_errorResponse(response, 403);

	std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	in.close();

	response->setStatus(200, "OK");
	response->setBody(body, get_content_type(file_path));

}

void RequestHandler::_handleAutoindex(HTTPResponse* response, const std::string& dir_path) {

	DIR* dir = opendir(dir_path.c_str());

	if (!dir)
		_errorResponse(response, 403);

	struct dirent* entry;
	std::string body = "<html><body><h1>Index of " + _request->getPath() + "</h1><ul>";

	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name == ".") continue;
		body += "<li><a href=\"" + name + "\">" + name + "</a></li>";
	}

	body += "</ul></body></html>";

	closedir(dir);

	response->setStatus(200, "OK");
	response->setBody(body, "text/html");

}

void RequestHandler::_handleUpload(HTTPRequest* request, HTTPResponse* response) {

	if (_request->getMethod() != "POST")
		_errorResponse(response, 405);

	if (!_location)
		_errorResponse(response, 404);

	if (_request->getBody().empty())
		_errorResponse(response, 400);

	if (_request->getBody().size() > _config->client_max_body_size) {
		response->setStatus(413, "Payload Too Large");
	}

	struct stat st;
	std::string upload_dir = _location->upload_dir.empty() ? _location->root : _location->upload_dir;

	if (stat(upload_dir.c_str(), &st) == -1 || !S_ISDIR(st.st_mode))
		_errorResponse(response, 403);

	std::string out_path = upload_dir + "/upload.bin";
	std::ofstream out(out_path.c_str(), std::ios::binary);

	if (!out.is_open())
		_errorResponse(response, 500);

	out << _request->getBody();
	out.close();

	response->setStatus(201, "Created");
	response->setBody("Uploaded\n", "text/plain");

}

void RequestHandler::_handleDelete(HTTPRequest* request, HTTPResponse* response) {

	if (_request->getMethod() != "DELETE")
		_errorResponse(response, 405);

	if (!_location)
		_errorResponse(response, 404);

	struct stat st;
	std::string file_path = _location->root + _request->getPath();

	if (stat(file_path.c_str(), &st) == -1)
		_errorResponse(response, 404);

	if (!S_ISREG(st.st_mode))
		_errorResponse(response, 403);

	if (unlink(file_path.c_str()) == -1)
		_errorResponse(response, 500);

	response->setStatus(204, "No Content");

}


void RequestHandler::_errorResponse(HTTPResponse* response, int code) {

	std::string body;
	std::map<int, std::string>::const_iterator it = _config->error_pages.find(code);

	response->setStatus(code);

	if (it != _config->error_pages.end()) {
		std::string page_path = it->second;

		if (!page_path.empty() && page_path[0] != '/')
			page_path = _config->root + "/" + page_path;

		std::ifstream in(page_path.c_str(), std::ios::binary);

		if (in.is_open()) {
			body.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
			in.close();
			response->setBody(body, get_content_type(it->second));
		}

	}

	body = "<html><body><h1>" + response->getReasonPhrase() + "</h1></body></html>";

	response->setBody(body, "text/html");

}
