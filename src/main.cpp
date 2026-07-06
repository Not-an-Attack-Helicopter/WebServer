#include "../includes/config_parser.hpp"
#include "../includes/server.hpp"
#include "../includes/colors.hpp"
#include "../includes/utils.hpp"
#include <iostream>

void deleteServer(ConfigParser* parser, Server* server) {
	delete parser;
	parser = NULL;
	delete server;
	server = NULL;
}

int main(int ac, char** av) {

	std::cout	<< DEBUG << "Parsing Configuration file..."
				<< RESET<< std::endl;

	// std::string configPath = av[2]; // makes valgrind go crazy seeing leaks where there aren't'

	ConfigParser* parser = NULL;
	switch (ac) {
	case 3:
		if (std::string(av[1]) == "-v") {
			std::cout << INFO << "Using: " << av[2] << RESET << std::endl;
			try {
				parser = new ConfigParser(av[2]);
			} catch (std::runtime_error& e) {
				std::cerr << ERROR << e.what() << RESET << std::endl;
				delete parser;
				return 1;
			}
			print_conf(parser->getAllConfigs());
		} else if (std::string(av[2]) == "-v") {
			std::cout << INFO << "Using: " << av[1] << RESET << std::endl;
			try {
				parser = new ConfigParser(av[1]);
			} catch (std::runtime_error& e) {
				std::cerr << ERROR << e.what() << RESET << std::endl;
				delete parser;
				return 1;
			}
		} else {
			std::cerr << "Usage: " << av[0] << " [-v] <config_file>" << std::endl;
			return 0;
		}
		break;
	case 2:
		std::cout << INFO << "Using: " << av[1] << RESET << std::endl;
		try {
			parser = new ConfigParser(av[1]);
		} catch (std::runtime_error& e) {
			std::cerr << ERROR << e.what() << RESET << std::endl;
			delete parser;
			return 1;
		}
		break;
	case 1:
		std::cout << INFO << "Using Default Configuration" << RESET << std::endl;
		try {
			parser = new ConfigParser("Config_Files/default.conf");
		} catch (std::runtime_error& e) {
			std::cerr << ERROR << e.what() << RESET << std::endl;
			delete parser;
			return 1;
		}
		break;
	default:
		std::cerr << "Usage: " << av[0] << " [-v] <config_file>" << std::endl;
		return 0;
	}

	size_t numSockets = parser->getServerConfigCount();
	if (numSockets == 0) {
		std::cerr	<< ERROR << "Error: No configuration provided"
					<< RESET << std::endl;
		delete parser;
		return 1;
	}

	Server* server = new Server();
	server->setConfig(parser->getAllConfigs());

	// Setup phase: create the epoll instance and all listening sockets
	try {
		server->prepareEPollInstance();
	} catch (const std::exception& e) {
		std::cerr << ERROR << e.what() << RESET << std::endl;
		deleteServer(parser, server);
		return 1;
	}
	for (size_t i = 0; i < numSockets; ++i) {
		try {
			const ServerConfig& cfg = parser->getSingleConfig(i);
			std::string address = cfg.host;
			unsigned short port = cfg.port;
			server->prepareListeningPort(address, port, &cfg);
		} catch (const std::exception& e) {
			std::cerr << ERROR << e.what() << RESET << std::endl;
			deleteServer(parser, server);
			return 1;
		}
	}
	// Run phase: listen on all sockets for events
	try {
		server->handleIncomingEvents();
	} catch (const std::exception& e) {
		std::cerr << ERROR << e.what() << RESET << std::endl;
		deleteServer(parser, server);
		return 1;
	}
	deleteServer(parser, server);
	return 0;
}