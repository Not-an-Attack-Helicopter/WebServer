#include "requesthandler.hpp"

//Constructors, assignment operator, destructor
RequestHandler::RequestHandler(const ServerConfig& server, const HTTPRequest& req)
	: _server(server), _req(req), _location(_match_location()) {}

RequestHandler::RequestHandler(const RequestHandler& other)
	: _server(other._server), _req(other._req), _location(other._location) {}

RequestHandler& RequestHandler::operator=(const RequestHandler& other)
{
	if (this != &other)
	{
		_location = other._location;
	}
	return *this;
}

RequestHandler::~RequestHandler() {}

//Private methods

const LocationConfig*   RequestHandler::_match_location() const
{
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

HTTPResponse    RequestHandler::_handle_redirect()
{
	HTTPResponse res;
	res.setStatus(301, "Moved Permanently");
	res.setHeader("Location", _location->redirect);
	return res;
}

HTTPResponse    RequestHandler::_handle_static()
{
	HTTPResponse res;
	if(_req.getMethod() != "GET")
		return _error_response(405);
	if (!_location)
		return _error_response(404);

	// Use location root, fall back to server root
	std::string root = _location->root.empty() ? _server.root : _location->root;
	std::string file_path = root + _req.getPath();

	struct stat st;
	if (stat(file_path.c_str(), &st) == -1)
		return _error_response(404);
	if (S_ISDIR(st.st_mode))
	{
		std::string index_file = _location->index.empty() ? _server.index : _location->index;
		if (!index_file.empty())
		{
			std::string index_path = file_path + "/" + index_file;
			if (stat(index_path.c_str(), &st) == -1 || !S_ISREG(st.st_mode))
			{
				if (_location->autoindex)
					return _handle_autoindex(file_path);
				return _error_response(404);
			}
			file_path = index_path;
		}
		else if (_location->autoindex)
			return _handle_autoindex(file_path);
		else
			return _error_response(403);
	}
	std::ifstream in(file_path.c_str(), std::ios::binary);
	if (!in.is_open())
		return _error_response(403);
	std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	in.close();
	res.setStatus(200, "OK");
	res.setBody(body, get_content_type(file_path));
	return res;
}
HTTPResponse    RequestHandler::_handle_autoindex(const std::string& dir_path)
{
	HTTPResponse res;
	DIR* dir = opendir(dir_path.c_str());
	if (!dir)
		return _error_response(403);
	std::string body = "<html><body><h1>Index of " + _req.getPath() + "</h1><ul>";
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		if (name == ".") continue;
		body += "<li><a href=\"" + name + "\">" + name + "</a></li>";
	}
	body += "</ul></body></html>";
	closedir(dir);
	res.setStatus(200, "OK");
	res.setBody(body, "text/html");
	return res;

}
HTTPResponse    RequestHandler::_handle_upload()
{
	HTTPResponse res;
	if (_req.getMethod() != "POST")
		return _error_response(405);
	if (!_location)
		return _error_response(404);
	if (_req.getBody().empty())
		return _error_response(400);
	if (_req.getBody().size() > _server.client_max_body_size)
	{
		res.setStatus(413, "Payload Too Large");
		return res;
	}

	std::string upload_dir = _location->upload_dir;
	if (upload_dir.empty())
	{
		if (_location->root.empty())
			upload_dir = _server.root;
		else
			upload_dir = _location->root;
	}
	struct stat st;
	if (stat(upload_dir.c_str(), &st) == -1 || !S_ISDIR(st.st_mode))
		return _error_response(403);

	std::string out_path = upload_dir + "/upload.bin";
	std::ofstream out(out_path.c_str(), std::ios::binary);
	if (!out.is_open())
		return _error_response(500);
	out << _req.getBody();
	out.close();

	res.setStatus(201, "Created");
	res.setBody("Uploaded\n", "text/plain");
	return res;
}
HTTPResponse    RequestHandler::_handle_delete()
{
	HTTPResponse res;
	if (_req.getMethod() != "DELETE")
		return _error_response(405);
	if (!_location)
		return _error_response(404);

	std::string root = _location->root.empty() ? _server.root : _location->root;
	std::string file_path = root + _req.getPath();
	struct stat st;
	if (stat(file_path.c_str(), &st) == -1)
		return _error_response(404);
	if (!S_ISREG(st.st_mode))
		return _error_response(403);
	if (unlink(file_path.c_str()) == -1)
		return _error_response(500);

	res.setStatus(204, "No Content");
	return res;
}


HTTPResponse    RequestHandler::_error_response(int code)
{
	HTTPResponse res;
	res.setStatus(code);
	std::string body;

	std::map<int, std::string>::const_iterator it = _server.error_pages.find(code);
	if (it != _server.error_pages.end())
	{
		std::string page_path = it->second;
		// Try the path as-is first (relative to CWD)
		std::ifstream in(page_path.c_str(), std::ios::binary);
		if (!in.is_open() && !page_path.empty() && page_path[0] != '/')
		{
			// Fall back to server-root-relative
			page_path = _server.root + "/" + page_path;
			in.open(page_path.c_str(), std::ios::binary);
		}
		if (in.is_open())
		{
			body.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
			in.close();
			res.setBody(body, get_content_type(page_path));
			return res;
		}
	}
	body = "<html><body><h1>" + res.getReasonPhrase() + "</h1></body></html>";
	res.setBody(body, "text/html");
	return res;
}


//Public method

HTTPResponse    RequestHandler::handler()
{
	if (_location && !_location->redirect.empty())
		return _handle_redirect();
	if (_location && !_location->cgi_extension.empty())
		return _error_response(501);
	if (_req.getMethod() == "DELETE")
		return _handle_delete();
	if (_req.getMethod() == "POST" && _location && !_location->upload_dir.empty())
		return _handle_upload();
	return _handle_static();
}