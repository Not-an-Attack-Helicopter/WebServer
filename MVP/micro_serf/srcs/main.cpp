/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 12:43:04 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/05 12:43:06 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/Logger.hpp"
#include "../incs/Parser.hpp"
#include "../incs/Server.hpp"
#include "../incs/utils.hpp"
#include <exception>
#include <cstring>
// #include <iostream>
// #include "../incs/colors.hpp"

int main(int argc, char** argv) {

	std::string config;
	std::string av[argc];
	for (int i = 0; i <= argc; ++i) {
		if (argv[i] != NULL) av[i] = argv[i];
	}

	// Prepare phase: interpret provided arguments and proceed accordingly
	switch (argc) {
	case 4:
		if (av[1] == "-l" || av[1] == "--log-level") {
			log.setLevel(av[2]);
			config = av[3];
		} else if (av[2] == "-l" || av[2] == "--log-level") {
			log.setLevel(av[3]);
			config = av[1];
		} else {
			log.notice("Usage: " + av[0] + " [-v] <config_file>");
			return 0;
		}
		break;
	case 3:
		if (av[1] == "-q" || av[1] == "--quiet") {
			log.setLevel(LOG_LEVEL_OFF);
			config = av[2];
		} else if (av[2] == "-q" || av[2] == "--quiet") {
			log.setLevel(LOG_LEVEL_OFF);
			config = av[1];
		} else if (av[1] == "-l" || av[1] == "--log-level") {
			log.setLevel(av[2]);
			// log.info("Using Default Configuration");
			config = "Config_Files/default.conf";
		} else {
			log.notice("Usage: " + av[0] + " [-v] <config_file>");
			return 0;
		}
		break;
	case 2:
		if (av[1] == "-q" || av[1] == "--quiet") {
			log.setLevel(LOG_LEVEL_OFF);
			// log.info("Using Default Configuration");
			config = "Config_Files/default.conf";
		} else {
			config = av[1];
		}
		break;
	case 1:
		// log.info("Using Default Configuration");
		config = "Config_Files/default.conf";
		break;
	default:
		log.notice("Usage: " + av[0] + " [-v] <config_file>");
		return 0;
	}
	log.info("Using configuration file: " + config);

	// Parse phase: parse through config file and extract server setup
	size_t numSockets = 0;
	try {
		parser.readFile(config);
		log.debug("Parsing configuration file: " + config);
		numSockets = parser.countConfigs();
		if (numSockets == 0) {
			log.error("Error: No configuration provided");
			return 1;
		}
		if (log.getLevel() <= LOG_LEVEL_INFO) {
			print_conf(parser.getAllConfigs());
		}
	} catch (std::exception& e) {
		log.error(e.what());
		return 1;
	}


	// Setup phase: create server, epoll instance, and all listening sockets
	// Server* server = NULL;
	// try {
	// 	server = new Server();
	// } catch (std::exception& e) {
	// 	// std::cerr << ERROR << e.what() << RESET << std::endl;
	// 	log.error(e.what());
	// 	delete server;
	// 	return 1;
	// }
	try {
		server.prepareEPollInstance();
	} catch (const std::exception& e) {
		// std::cerr << ERROR << e.what() << RESET << std::endl;
		log.error(e.what());
		return 1;
	}
	for (size_t i = 0; i < numSockets; ++i) {
		try {
			std::string address = parser.getConfig(i).host;
			unsigned short port = parser.getConfig(i).port;
			server.prepareListeningPort(address, port);
		} catch (const std::exception& e) {
			// std::cerr << ERROR << e.what() << RESET << std::endl;
			log.error(e.what());
			return 1;
		}
	}

	// Run phase: listen on all sockets for events
	try {
		server.handleIncomingEvents();
	} catch (const std::exception& e) {
		// std::cerr << ERROR << e.what() << RESET << std::endl;
		log.error(e.what());
		return 1;
	}
	return 0;
}
