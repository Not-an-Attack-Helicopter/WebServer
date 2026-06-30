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
RequestHandler::RequestHandler(const Config& server, const HTTPRequest& req)
	:	_server(server), _req(req), _location(_match_location()) {
	return;
}

/*	@brief Destructor	*/
RequestHandler::~RequestHandler(void) {
	return;
}

/*	@brief Copy Constructor	*/
RequestHandler::RequestHandler(const RequestHandler& other)
	:	_server(other._server), _req(other._req), _location(other._location) {
	return;
}

/*	@brief Copy Assignment Operator	*/
RequestHandler& RequestHandler::operator = (const RequestHandler& other) {
	if (this != &other) {
		_location = other._location;
	}
	return *this;
}

void RequestHandler::handler(HTTPResponse* response) {

	if (_location && !_location->redirect.empty())
		_handle_redirect(response);
	else if (_location && !_location->cgi_extension.empty())
		_handle_static(response);
	else if (_location && _location->autoindex)
		_handle_autoindex(response, _location->root + _req.getPath());
	else if (_location && !_location->upload_dir.empty())
		_handle_upload(response);
	else if (_location && std::find(_location->methods.begin(), _location->methods.end(), "DELETE") != _location->methods.end())
		_handle_delete(response);
	else
		_handle_static(response);

}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

const LocationConfig* RequestHandler::_match_location() const {

	const LocationConfig* best = NULL;
	size_t best_len = 0;
	const std::string& path = _req.getPath();

	for (size_t i = 0; i < _server.locations.size(); ++i)
	{
		const LocationConfig& loc = _server.locations[i];
		if (path.compare(0, loc.path.size(), loc.path) == 0)
		{
			if (loc.path.size() > best_len)
			{
				best = &_server.locations[i];
				best_len = loc.path.size();
			}
		}
	}
	return best;

}

void RequestHandler::_handle_redirect(HTTPResponse* response) {

	response->setStatus(301, "Moved Permanently");
	response->setHeader("Location", _location->redirect);

}

void RequestHandler::_handle_static(HTTPResponse* response) {

	if(_req.getMethod() != "GET")
		_error_response(response, 405);

	if (!_location)
		_error_response(response, 404);

	struct stat st;
	std::string file_path = _location->root + _req.getPath();

	if (stat(file_path.c_str(), &st) == -1)
		_error_response(response, 404);

	if (S_ISDIR(st.st_mode)) {

		if (_location->index.empty())
			_handle_autoindex(response, file_path);
		file_path += "/" + _location->index;

		if (stat(file_path.c_str(), &st) == -1 || !S_ISREG(st.st_mode))
			_error_response(response, 404);

	}

	std::ifstream in(file_path.c_str(), std::ios::binary);

	if (!in.is_open())
		_error_response(response, 403);

	std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	in.close();

	response->setStatus(200, "OK");
	response->setBody(body, get_content_type(file_path));

}

void RequestHandler::_handle_autoindex(HTTPResponse* response, const std::string& dir_path) {

	DIR* dir = opendir(dir_path.c_str());

	if (!dir)
		_error_response(response, 403);

	struct dirent* entry;
	std::string body = "<html><body><h1>Index of " + _req.getPath() + "</h1><ul>";

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

void RequestHandler::_handle_upload(HTTPResponse* response) {

	if (_req.getMethod() != "POST")
		_error_response(response, 405);

	if (!_location)
		_error_response(response, 404);

	if (_req.getBody().empty())
		_error_response(response, 400);

	if (_req.getBody().size() > _server.client_max_body_size) {
		response->setStatus(413, "Payload Too Large");
	}

	struct stat st;
	std::string upload_dir = _location->upload_dir.empty() ? _location->root : _location->upload_dir;

	if (stat(upload_dir.c_str(), &st) == -1 || !S_ISDIR(st.st_mode))
		_error_response(response, 403);

	std::string out_path = upload_dir + "/upload.bin";
	std::ofstream out(out_path.c_str(), std::ios::binary);

	if (!out.is_open())
		_error_response(response, 500);

	out << _req.getBody();
	out.close();

	response->setStatus(201, "Created");
	response->setBody("Uploaded\n", "text/plain");

}

void RequestHandler::_handle_delete(HTTPResponse* response) {

	if (_req.getMethod() != "DELETE")
		_error_response(response, 405);

	if (!_location)
		_error_response(response, 404);

	struct stat st;
	std::string file_path = _location->root + _req.getPath();

	if (stat(file_path.c_str(), &st) == -1)
		_error_response(response, 404);

	if (!S_ISREG(st.st_mode))
		_error_response(response, 403);

	if (unlink(file_path.c_str()) == -1)
		_error_response(response, 500);

	response->setStatus(204, "No Content");

}


void RequestHandler::_error_response(HTTPResponse* response, int code) {

	std::string body;
	std::map<int, std::string>::const_iterator it = _server.error_pages.find(code);

	response->setStatus(code);

	if (it != _server.error_pages.end()) {
		std::string page_path = it->second;

		if (!page_path.empty() && page_path[0] != '/')
			page_path = _server.root + "/" + page_path;

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
