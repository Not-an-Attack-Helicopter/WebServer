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
// #include "../incs/utils.hpp"
// #include "../incs/Client.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
// #include <cstdlib>

// void shutdownServer(ConfigParser* parser, Server* server) {
// 	// server->cleanUpAllRessources(count);
// 	delete parser;
// 	parser = NULL;
// 	delete server;
// 	server = NULL;
// }

void deleteServer(Server* server) {
	// server->cleanUpAllRessources(count);
	delete server;
	server = NULL;
}

// int main(void) {
int main(int ac, char** av) {

	std::cout << "Parsing Configuration file..." << std::endl;

	ConfigParser* parser;
	switch (ac) {
	case 3:
		if (std::string(av[1]) == "-v") {
			std::cout << "Using: " << av[2] << std::endl;
			// arg = 2;
			// ConfigParser p(av[2]);
			// parser = &p;
			parser = new ConfigParser(av[2]);
			// configPath = av[2]; // makes valgrind go crazy seeing leaks where there aren't'
		} else if (std::string(av[2]) == "-v") {
			std::cout << "Using: " << av[1] << std::endl;
			// arg = 1;
			parser = new ConfigParser(av[1]);
			// ConfigParser p(av[1]);
			// parser = &p;
			// configPath = av[1];
		}
		break;
	case 2:
		std::cout << "Using: " << av[1] << std::endl;
		// arg = 1;
		parser = new ConfigParser(av[1]);
		// ConfigParser p(av[1]);
		// parser = &p;
		// configPath = av[1];
		break;
	default:
		std::cout << "Using Default Configuration" << std::endl;
	// 	arg = 0;
		parser = new ConfigParser("Config_Files/default.conf");
		// ConfigParser parser("Config_Files/default.conf");
		// parser = &p;
		// configPath = "Config_Files/default.conf";
		// break;
	}

	// if (arg) {
	// 	ConfigParser parser(av[arg]);
	// }

	// if (ac == 3) {
	// 	if (std::string(av[1]) == "-v") {
	// 		std::cout << "Using: " << av[2] << std::endl;
	// 		ConfigParser parser(av[2]);
	// 	} else if (std::string(av[2]) == "-v") {
	// 			ConfigParser parser(av[1]);
	// 	}
	// } else if (ac == 2) {
	// 	ConfigParser parser(av[1]);
	// } else {
	// 	ConfigParser("Config_Files/default.conf");
	// }
	// ConfigParser parser("Config_Files/default.conf");
	// if (arg) {
	// 	ConfigParser parser(av[arg]);
	// }

	size_t n = parser->getServerConfigCount();
	// size_t n = 2;
	if (n == 0) {
		std::cerr << "Error: No configuration provided" << std::endl;
		return 1;
	}
	// Server** servers = new Server*[n];
	Server* server = new Server();
	size_t count = 0;
	size_t i = -1;
	while (++i < n) {
		count = i;
		// servers[i] = new Server();
		// std::string address = parser.get_config()[i].host;
		std::string address = parser->getServerConfig(i).host;
		// unsigned short port = parser.get_config()[i].port;
		unsigned short port = parser->getServerConfig(i).port;

		// std::string address = "127.0.0.1";
		// unsigned short port = 8325 + (i * 111);

		try {
			server->prepareListeningPort(i, address, port);
		} catch (Server::SocketException& e) {
			std::cerr	<< "\e[31mError: socket: " << e.what()
						<< "\e[0m" << std::endl;
			// shutdownServer(parser, server);
			deleteServer(server);
			return 10;
		} catch (Server::BindException& e) {
			std::cerr	<< "\e[31mError: bind: " << e.what()
						<< "\e[0m" << std::endl;
			// shutdownServer(parser, server);
			deleteServer(server);
			return 11;
		} catch (Server::ListenException& e) {
			std::cerr	<< "\e[31mError: listen: " << e.what()
						<< "\e[0m" << std::endl;
			// shutdownServer(parser, server);
			deleteServer(server);
			return 12;
		} catch (const std::exception& e) {
			std::cerr	<< "\e[31mError: No server created. " << e.what()
						<< "\e[0m" << std::endl;
			// shutdownServer(parser, server);
			deleteServer(server);
			return 13;
		}

		try {
			server->prepareEPollInstance(i);
		} catch (Server::CreateEPollException& e) {
			std::cerr	<< "\e[31mError: epoll_create " << e.what()
						<< "\e[0m" << std::endl;
			// shutdownServer(parser, server);
			deleteServer(server);
			return 21;
		} catch (Server::ModifyEPollException& e) {
			std::cerr	<< "\e[31mError: epoll_ctl: " << e.what()
						<< "\e[0m" << std::endl;
			// shutdownServer(parser, server);
			deleteServer(server);
			return 22;
		} catch (const std::exception& e) {
			std::cerr	<< "\e[31mError: No epoll instance created. " << e.what()
						<< "\e[0m" << std::endl;
			// shutdownServer(parser, server);
			deleteServer(server);
			return 23;
		}
	}
	i = -1;
	while (++i < (count + 1)) {
		pid_t pid = fork();
		switch(pid) {
		case -1: // Error
			std::cerr << "Error: fork: " << strerror(errno) << std::endl;
			// shutdownServer(parser, server);
			deleteServer(server);
			return 1;
		case 0: // Child
			try {
				server->handleIncomingEvents(i);
			} catch (Server::EventPollingException& e) {
				std::cerr	<< "\e[31mError: epoll_wait: " << e.what()
							<< "\e[0m" << std::endl;
				// shutdownServer(parser, server);
				deleteServer(server);
				exit(1);
				// return 31;
			} catch (Server::AcceptException& e) {
				std::cerr	<< "\e[31mError: accept: " << e.what()
							<< "\e[0m" << std::endl;
				// shutdownServer(parser, server);
				deleteServer(server);
				exit(1);
				// return 32;
			} catch (Server::GetFlagsException& e) {
				std::cerr	<< "\e[31mError: fcntl(F_GETFL): " << e.what()
							<< "\e[0m" << std::endl;
				// shutdownServer(parser, server);
				deleteServer(server);
				exit(1);
				// return 33;
			} catch (Server::SetFlagsException& e) {
				std::cerr	<< "\e[31mError: fcntl(F_SETFL): " << e.what()
							<< "\e[0m" << std::endl;
				// shutdownServer(parser, server);
				deleteServer(server);
				exit(1);
				// return 34;
			} catch (Server::ModifyEPollException& e) {
				std::cerr	<< "\e[31mError: epoll_ctl: " << e.what()
							<< "\e[0m" << std::endl;
				// shutdownServer(parser, server);
				deleteServer(server);
				exit(1);
				// return 35;
			} catch (Server::ReadDataException& e) {
				std::cerr	<< "\e[31mError: recv: " << e.what()
							<< "\e[0m" << std::endl;
				// shutdownServer(parser, server);
				deleteServer(server);
				exit(1);
				// return 36;
			} catch (Server::FlushDataException& e) {
				std::cerr	<< "\e[31mError: send: " << e.what()
							<< "\e[0m" << std::endl;
				// shutdownServer(parser, server);
				deleteServer(server);
				exit(1);
				// return 37;
			} catch (const std::exception& e) {
				std::cerr	<< "\e[31mError: Event handling failed. " << e.what()
							<< "\e[0m" << std::endl;
				// shutdownServer(parser, server);
				deleteServer(server);
				exit(1);
				// return 38;
			}
			delete parser;
			// shutdownServer(parser, server);
			deleteServer(server);
			exit(0);
		}
	}
	int status; // Exit program
	while (wait(&status) > 0) {
		// dprintf(STDERR_FILENO, "\tPARENT(%i):\twaiting for CHILD(%i) - pid = %i...\n", i, i, pid);
		if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
			// dprintf(STDERR_FILENO, "\tCHILD(%i):\texited with failure\n", i);
			return 1;
		// } else {
		// 	dprintf(STDERR_FILENO, "\tCHILD(%i):\texited with success\n", i);
		}
		--i;
		// --pid;
	}
	delete parser;
	// shutdownServer(parser, server);
	deleteServer(server);
	return 0;
}


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
// 			std::cerr	<< "\e[31mError: epoll_wait: " << e.what()
// 						<< "\e[0m" << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(31);
// 		} catch (Server::AcceptException& e) {
// 			std::cerr	<< "\e[31mError: accept: " << e.what()
// 						<< "\e[0m" << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(32);
// 		} catch (Server::GetFlagsException& e) {
// 			std::cerr	<< "\e[31mError: fcntl(F_GETFL): " << e.what()
// 						<< "\e[0m" << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(33);
// 		} catch (Server::SetFlagsException& e) {
// 			std::cerr	<< "\e[31mError: fcntl(F_SETFL): " << e.what()
// 						<< "\e[0m" << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(34);
// 		} catch (Server::ModifyEPollException& e) {
// 			std::cerr	<< "\e[31mError: epoll_ctl: " << e.what()
// 						<< "\e[0m" << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(35);
// 		} catch (Server::ReadDataException& e) {
// 			std::cerr	<< "\e[31mError: recv: " << e.what()
// 			// std::cout	<< "\e[31mServer shutdown initiated"
// 						<< "\e[0m" << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(36);
// 		} catch (Server::FlushDataException& e) {
// 			std::cerr	<< "\e[31mError: send: " << e.what()
// 						<< "\e[0m" << std::endl;
// 			servers[i]->cleanUpAllRessources();
// 			delete servers[i];
// 			servers[i] = NULL;
// 			exit(37);
// 		} catch (const std::exception& e) {
// 			std::cerr	<< "\e[31mError: Event handling failed. " << e.what()
// 						<< "\e[0m" << std::endl;
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
// 	std::cerr	<< "\e[31mError: socket: " << e.what()
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 10;
// } catch (Server::BindException& e) {
// 	std::cerr	<< "\e[31mError: bind: " << e.what()
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 11;
// } catch (Server::ListenException& e) {
// 	std::cerr	<< "\e[31mError: listen: " << e.what()
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 12;
// } catch (const std::exception& e) {
// 	std::cerr	<< "\e[31mError: No server created. " << e.what()
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 13;
// }

// try {
// 	server->prepareEPollInstance();
// } catch (Server::CreateEPollException& e) {
// 	std::cerr	<< "\e[31mError: epoll_create " << e.what()
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 21;
// } catch (Server::ModifyEPollException& e) {
// 	std::cerr	<< "\e[31mError: epoll_ctl: " << e.what()
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 22;
// } catch (const std::exception& e) {
// 	std::cerr	<< "\e[31mError: No epoll instance created. " << e.what()
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 23;
// }

// try {
// 	server->handleIncomingEvents();
// } catch (Server::EventPollingException& e) {
// 	std::cerr	<< "\e[31mError: epoll_wait: " << e.what()
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 31;
// } catch (Server::AcceptException& e) {
// 	std::cerr	<< "\e[31mError: accept: " << e.what()
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 32;
// } catch (Server::GetFlagsException& e) {
// 	std::cerr	<< "\e[31mError: fcntl(F_GETFL): " << e.what()
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 33;
// } catch (Server::SetFlagsException& e) {
// 	std::cerr	<< "\e[31mError: fcntl(F_SETFL): " << e.what()
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 34;
// } catch (Server::ModifyEPollException& e) {
// 	std::cerr	<< "\e[31mError: epoll_ctl: " << e.what()
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 35;
// } catch (Server::ReadDataException& e) {
// 	std::cerr	<< "\e[31mError: recv: " << e.what()
// 	// std::cout	<< "\e[31mServer shutdown initiated"
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 36;
// } catch (Server::FlushDataException& e) {
// 	std::cerr	<< "\e[31mError: send: " << e.what()
// 				<< "\e[0m" << std::endl;
// 	server->cleanUpAllRessources();
// 	delete server;
// 	return 37;
// } catch (const std::exception& e) {
// 	std::cerr	<< "\e[31mError: Event handling failed. " << e.what()
// 				<< "\e[0m" << std::endl;
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
