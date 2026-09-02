static std::string readCgiInput(const HTTPRequest& request) {

	if (request.body.sink == DISK) {

		std::string content;
		content.resize(request.body.size);
		char* ptr = &content[0];

		ssize_t bytes_read;
		ssize_t bytes_left = request.body.size;
		do {

			bytes_read = read(fd, ptr, bytes_left);
			ptr += bytes_read;
			bytes_left -= bytes_read;

		} while (bytes_read > 0);

		if (bytes_read == 0) {

			return content;

		} else {

			return ("");

		}

	} else if (request.body.sink == HEAP){

		if (request.body.temp.empty())
			return "";

		return request.body.temp.str();

	} else {

		return ("");

	}

}

static void readCgiInput(const HTTPRequest& request) {

	request.body.temp.resize(request.body.size);
	char* ptr = &request.body.temp[0];

	ssize_t bytes_read;
	ssize_t bytes_left = request.body.size;
	do {

		bytes_read = read(request.body.file, ptr, bytes_left);
		ptr += bytes_read;
		bytes_left -= bytes_read;

	} while (bytes_read > 0);
	close(request.body.file);

	if (bytes_read != 0) {
		request.body.temp = "";
	}

}

StatusCode handleCGI(HTTPRequest& request, HTTPResponse& response,
					 const Config::Socket& socket) {

	(void)response; // filled in once the CGI actually runs

	if (request.body.sink == DISK) {
		readCgiInput(request);
	}
	std::vector<std::string> cgi_args = buildCgiArgs(request);
	std::string working_dir = request.resolved.path.substr(0, request.resolved.path.find_last_of('/'));

	std::map<std::string, std::string> env = build_cgi_env(request, socket,
															*request.resolved.domain,
															*request.resolved.location,
															request.resolved.path);

	request.cgi_process = new CgiProcess(request.cgi.binary_path, cgi_args, env, request.body.temp, working_dir);

	if (!request.cgi_process->valid()) {
		log.error("cgi: failed to open pipes for " + request.cgi.binary_path);
		return INTERNAL_SERVER_ERROR;
	}

	return NO_STATUS;

}
