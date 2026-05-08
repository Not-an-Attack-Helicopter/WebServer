#include "webserver.hpp"


void print_conf(const Config& config) {
	for (const auto& server : config.servers) {
		std::cout << "server {\n";
		std::cout << "    host: " << server.host << ";\n";
		std::cout << "    port: " << server.port << ";\n";
		std::cout << "    client_max_body_size: " << server.client_max_body_size << ";\n";
			std::cout << "    error_pages {\n";
		for (const auto& error_page : server.error_pages) {
			std::cout << "        " << error_page.first << " " << error_page.second << ";\n";
		}
		std::cout << "    }\n";
			std::cout << "    locations {\n";
		for (const auto& location : server.locations) {
			std::cout << "        location " << location.path << " {\n";
			std::cout << "            methods: [";
			for (size_t i = 0; i < location.methods.size(); ++i) {
				std::cout << location.methods[i];
				if (i < location.methods.size() - 1) std::cout << ", ";
			}
			std::cout << "];\n";
			std::cout << "            root: " << location.root << ";\n";
			std::cout << "            index: " << location.index << ";\n";
			std::cout << "            redirect: " << location.redirect << ";\n";
			std::cout << "            autoindex: " << (location.autoindex ? "on" : "off") << ";\n";
			std::cout << "            upload_dir: " << location.upload_dir << ";\n";
			std::cout << "            cgi_extension: " << location.cgi_extension << ";\n";
			std::cout << "            cgi_path: " << location.cgi_path << ";\n";
			std::cout << "        }\n";
		}
		std::cout << "    }\n";
	std::cout << "}\n";
	}
}

int main(int argc, char **argv)
{

	std::cout << "Starting WebServer..." << std::endl;
	// webserver server;
	if(argc >= 2)
	{
		std::cout << "Using: " << argv[1] << std::endl;
		//server.start(argv[1]);
	}
	else if (argc >= 3 && std::string(argv[1]) == "-c")
	{
		std::cout << "Using: " << argv[1] << std::endl;
		config_parser parser(argv[1]);
		print_conf(parser.get_config());
		//server.start(argv[1], argv[2]);
	}
	else
	{
		std::cout << "Using Default Configuration" << std::endl;
		//server.start("Config_Files/default.conf");
	}

	return 0;
}