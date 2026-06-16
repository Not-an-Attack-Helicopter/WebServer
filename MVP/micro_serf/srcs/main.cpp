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

#include "../incs/ConfigParser.hpp"
#include "../incs/Server.hpp"
#include "../incs/colors.hpp"
#include "../incs/utils.hpp"
#include <iostream>

// void deleteServer(ConfigParser* parser, Server* server) {
// 	delete parser;
// 	parser = NULL;
// 	delete server;
// 	server = NULL;
// }

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

	// Server* server = new Server();

	// Setup phase: create the epoll instance and all listening sockets
	try {
		server.prepareEPollInstance();
	} catch (const std::exception& e) {
		std::cerr << ERROR << e.what() << RESET << std::endl;
		delete parser;
		return 1;
	}
	for (size_t i = 0; i < numSockets; ++i) {
		try {
			std::string address = parser->getSingleConfig(i).host;
			unsigned short port = parser->getSingleConfig(i).port;
			server.prepareListeningPort(address, port);
		} catch (const std::exception& e) {
			std::cerr << ERROR << e.what() << RESET << std::endl;
			delete parser;
			return 1;
		}
	}
	// Run phase: listen on all sockets for events
	try {
		server.handleIncomingEvents();
	} catch (const std::exception& e) {
		std::cerr << ERROR << e.what() << RESET << std::endl;
		delete parser;
		return 1;
	}
	delete parser;
	return 0;
}

// std::string address = "127.0.0.1";
// unsigned short port = 0;
// if (i == 0) {
// 	port  = 8087;
// } else {
// 	port  = 8088;
// }

// // Fork phase: spawn child processes
// for (size_t i = 0; i < n; ++i) {
// 	pid_t pid = fork();
// 	switch(pid) {
// 	case -1:
// 		std::cerr	<< ERROR << "Error: fork: " << strerror(errno)
// 					<< RESET << std::endl;
// 		deleteServer(parser, server);
// 		return 1;
// 	case 0:
// 		// Child: handle events for this server index
// 		try {
// 			server->handleIncomingEvents();
// 		} catch (const std::exception& e) {
// 			std::cerr << ERROR << e.what() << RESET << std::endl;
// 			exit(1);
// 		}
// 		exit(EXIT_SUCCESS);
// 	default:
// 		// Parent: do nothing, loop continues to fork next child
// 		break;
// 	}
// }

// // Wait phase: parent waits for all children
// int status;
// int failed = 0;
// for (size_t i = 0; i < n; ++i) {
// 	if (wait(&status) == -1) {
// 		std::cerr	<< ERROR << "Error: wait: " << strerror(errno)
// 					<< RESET << std::endl;
// 		failed = 1;
// 	} else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
// 		failed = 1;
// 	}
// }

// Cleanup phase: parent cleans up ONCE
// deleteServer(parser, server);
// return failed;

// 	int count = -1;
// 	int i = -1;
// 	while (++i < n) {
// 		count = i;
// 		std::string address = parser->getSingleConfig(i).host;
// 		unsigned short port = parser->getSingleConfig(i).port;
//
// 		try {
// 			server->prepareListeningPort(i, address, port);
// 		} catch (Server::AFNotSupportedException& e) {
// 			std::cerr	<< ERROR << "Error: inet_pton: " << e.what()
// 			<< RESET << std::endl;
// 			cleanupServerIndex(i, server);
// 			deleteServer(parser, server);
// 			return 10;
// 		} catch (Server::InvalidAddressException& e) {
// 			std::cerr	<< ERROR << "Error: inet_pton: " << e.what()
// 			<< RESET << std::endl;
// 			cleanupServerIndex(i, server);
// 			deleteServer(parser, server);
// 			return 10;
// 		} catch (Server::SocketException& e) {
// 			std::cerr	<< ERROR << "Error: socket: " << e.what()
// 						<< RESET << std::endl;
// 			cleanupServerIndex(i, server);
// 			deleteServer(parser, server);
// 			return 11;
// 		} catch (Server::BindException& e) {
// 			std::cerr	<< ERROR << "Error: bind: " << e.what()
// 						<< RESET << std::endl;
// 			cleanupServerIndex(i, server);
// 			deleteServer(parser, server);
// 			return 12;
// 		} catch (Server::ListenException& e) {
// 			std::cerr	<< ERROR << "Error: listen: " << e.what()
// 						<< RESET << std::endl;
// 			cleanupServerIndex(i, server);
// 			deleteServer(parser, server);
// 			return 13;
// 		} catch (const std::exception& e) {
// 			std::cerr	<< ERROR << "Error: No server created. " << e.what()
// 						<< RESET << std::endl;
// 			cleanupServerIndex(i, server);
// 			deleteServer(parser, server);
// 			return 14;
// 		}
//
// 		try {
// 			server->prepareEPollInstance(i);
// 		} catch (Server::CreateEPollException& e) {
// 			std::cerr	<< ERROR << "Error: epoll_create " << e.what()
// 						<< RESET << std::endl;
// 			cleanupServerIndex(i, server);
// 			deleteServer(parser, server);
// 			return 21;
// 		} catch (Server::ModifyEPollException& e) {
// 			std::cerr	<< ERROR << "Error: epoll_ctl: " << e.what()
// 						<< RESET << std::endl;
// 			cleanupServerIndex(i, server);
// 			deleteServer(parser, server);
// 			return 22;
// 		} catch (const std::exception& e) {
// 			std::cerr	<< ERROR << "Error: No epoll instance created. " << e.what()
// 						<< RESET << std::endl;
// 			cleanupServerIndex(i, server);
// 			deleteServer(parser, server);
// 			return 23;
// 		}
// 	}
// 	i = -1;
// 	while (++i <= count) {
//
// 		pid_t pid = fork();
// 		switch(pid) {
//
// 		case -1: // Error
//
// 			std::cerr	<< ERROR << "Error: fork: " << strerror(errno)
// 						<< RESET << std::endl;
// 			cleanupServerIndex(i, server);
// 			deleteServer(parser, server);
// 			return 1;
//
// 		case 0: // Child
//
// 			try {
// 				server->handleIncomingEvents(i);
// 			} catch (Server::EventPollingException& e) {
// 				std::cerr	<< ERROR << "Error: epoll_wait: " << e.what()
// 							<< RESET << std::endl;
// 				cleanupServerIndex(i, server);
// 				exit(31);
// 			} catch (Server::AcceptException& e) {
// 				std::cerr	<< ERROR << "Error: accept: " << e.what()
// 							<< RESET << std::endl;
// 				cleanupServerIndex(i, server);
// 				exit(32);
// 			} catch (Server::GetFlagsException& e) {
// 				std::cerr	<< ERROR << "Error: fcntl(F_GETFL): " << e.what()
// 							<< RESET << std::endl;
// 				cleanupServerIndex(i, server);
// 				exit(33);
// 			} catch (Server::SetFlagsException& e) {
// 				std::cerr	<< ERROR << "Error: fcntl(F_SETFL): " << e.what()
// 							<< RESET << std::endl;
// 				cleanupServerIndex(i, server);
// 				exit(34);
// 			} catch (Server::ModifyEPollException& e) {
// 				std::cerr	<< ERROR << "Error: epoll_ctl: " << e.what()
// 							<< RESET << std::endl;
// 				cleanupServerIndex(i, server);
// 				exit(35);
// 			} catch (Server::ReadDataException& e) {
// 				std::cerr	<< ERROR << "Error: recv: " << e.what()
// 							<< RESET << std::endl;
// 				cleanupServerIndex(i, server);
// 				exit(36);
// 			} catch (Server::FlushDataException& e) {
// 				std::cerr	<< ERROR << "Error: send: " << e.what()
// 							<< RESET << std::endl;
// 				cleanupServerIndex(i, server);
// 				exit(37);
// 			} catch (const std::exception& e) {
// 				std::cerr	<< ERROR << "Error: Event handling failed. " << e.what()
// 							<< RESET << std::endl;
// 				cleanupServerIndex(i, server);
// 				exit(38);
// 			}
// 			cleanupServerIndex(i, server);
// 			exit(EXIT_SUCCESS);
//
// 		// default: // Parent
//
// 			// k = -1;
// 			// while (++k <= i) {
// 			// 	server->cleanUpAllRessources(k);
// 			// }
// 			// server->cleanUpAllRessources(i);
//
// 		}
//
// 	}
//
// 	int status; // Exit program
// 	while (wait(&status) > 0) {
// 		// dprintf(STDERR_FILENO, "\tPARENT(%i):\twaiting for CHILD(%i) - pid = %i...\n", i, i, pid);
// 		if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
// 			// dprintf(STDERR_FILENO, "\tCHILD(%i):\texited with failure\n", i);
// 			return 1;
// 		// } else {
// 		// 	dprintf(STDERR_FILENO, "\tCHILD(%i):\texited with success\n", i);
// 		}
// 		--i;
// 		// --pid;
// 	}
//
// 	// Clean up remaining server resources
// 	i = -1;
// 	while (++i <= count) {
// 		cleanupServerIndex(i, server);
// 	}
//
// 	// Delete global objects ONCE
// 	deleteServer(parser, server);
// 	return 0;
// }

// // std::vector<ServerConfig>::iterator it = parser.get_config().begin();
// i = -1;
// while (++i < n) {
// 	pid_t pid = fork();
// 	switch (pid) {
// 	case -1: // Error
// 		std::cerr << "Error: fork: " << strerror(errno) << std::endl;
// 		i = -1;
// 		while (++i < n) {
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 		}
// 		delete[] servers;
// 		return 1;
// 	case 0: // Child
// 		try {
// 			servers[i]->handleIncomingEvents();
// 		} catch (Server::EventPollingException& e) {
// 			std::cerr	<< ERROR << "Error: epoll_wait: " << e.what()
// 						<< RESET << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(31);
// 		} catch (Server::AcceptException& e) {
// 			std::cerr	<< ERROR << "Error: accept: " << e.what()
// 						<< RESET << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(32);
// 		} catch (Server::GetFlagsException& e) {
// 			std::cerr	<< ERROR << "Error: fcntl(F_GETFL): " << e.what()
// 						<< RESET << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(33);
// 		} catch (Server::SetFlagsException& e) {
// 			std::cerr	<< ERROR << "Error: fcntl(F_SETFL): " << e.what()
// 						<< RESET << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(34);
// 		} catch (Server::ModifyEPollException& e) {
// 			std::cerr	<< ERROR << "Error: epoll_ctl: " << e.what()
// 						<< RESET << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(35);
// 		} catch (Server::ReadDataException& e) {
// 			std::cerr	<< ERROR << "Error: recv: " << e.what()
// 			// std::cout	<< ERROR << "Server shutdown initiated"
// 						<< RESET << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(36);
// 		} catch (Server::FlushDataException& e) {
// 			std::cerr	<< ERROR << "Error: send: " << e.what()
// 						<< RESET << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(37);
// 		} catch (const std::exception& e) {
// 			std::cerr	<< ERROR << "Error: Event handling failed. " << e.what()
// 						<< RESET << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(38);
// 		}
// 		servers[i]->cleanUpAllRessources();
// 		delete servers[i];
// 		servers[i] = NULL;
// 		exit(EXIT_SUCCESS);
// 	}
// }

// 	i = -1;
// 	while (++i < n) {
// 		if (servers[i]) {
// 				servers[i]->cleanUpAllRessources();
// 				delete servers[i];
// 				servers[i] = NULL;
// 		}
// 	}
// 	delete[] servers;
// 	return 0;
// }

// // Server server;
// Server* server = new Server();

// try {
// 	// server.prepareListeningPort();
// 	server->prepareListeningPort(address, port);
// } catch (Server::SocketException& e) {
// 	std::cerr	<< ERROR << "Error: socket: " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 10;
// } catch (Server::BindException& e) {
// 	std::cerr	<< ERROR << "Error: bind: " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 11;
// } catch (Server::ListenException& e) {
// 	std::cerr	<< ERROR << "Error: listen: " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 12;
// } catch (const std::exception& e) {
// 	std::cerr	<< ERROR << "Error: No server created. " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 13;
// }

// try {
// 	server->prepareEPollInstance();
// } catch (Server::CreateEPollException& e) {
// 	std::cerr	<< ERROR << "Error: epoll_create " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 21;
// } catch (Server::ModifyEPollException& e) {
// 	std::cerr	<< ERROR << "Error: epoll_ctl: " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 22;
// } catch (const std::exception& e) {
// 	std::cerr	<< ERROR << "Error: No epoll instance created. " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 23;
// }

// try {
// 	server->handleIncomingEvents();
// } catch (Server::EventPollingException& e) {
// 	std::cerr	<< ERROR << "Error: epoll_wait: " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 31;
// } catch (Server::AcceptException& e) {
// 	std::cerr	<< ERROR << "Error: accept: " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 32;
// } catch (Server::GetFlagsException& e) {
// 	std::cerr	<< ERROR << "Error: fcntl(F_GETFL): " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 33;
// } catch (Server::SetFlagsException& e) {
// 	std::cerr	<< ERROR << "Error: fcntl(F_SETFL): " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 34;
// } catch (Server::ModifyEPollException& e) {
// 	std::cerr	<< ERROR << "Error: epoll_ctl: " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 35;
// } catch (Server::ReadDataException& e) {
// 	std::cerr	<< ERROR << "Error: recv: " << e.what()
// 	// std::cout	<< ERROR << "Server shutdown initiated"
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 36;
// } catch (Server::FlushDataException& e) {
// 	std::cerr	<< ERROR << "Error: send: " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 37;
// } catch (const std::exception& e) {
// 	std::cerr	<< ERROR << "Error: Event handling failed. " << e.what()
// 				<< RESET << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 38;
// }

// server->cleanUpAllRessources();
// delete server;

// std::string address = "";
// if (ac > 1) {
// 	address = av[1];
// }
// unsigned short port = 0;
// if (ac > 2) {
// 	port = atoi(av[2]);
// }

// default:
// 	std::cout<< "waiting..." << std::endl;
// }

// i = -1;
// while (++i < n) {
// 	servers[i]->cleanUpAllRessources();
// 	delete servers[i];
// }
// delete[] servers;
