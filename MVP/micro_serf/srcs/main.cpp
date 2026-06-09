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

#include "../incs/Server.hpp"
// #include "../incs/Client.hpp"
#include <fcntl.h>
#include <iostream>

// int main(void) {
int main(int ac, char** av) {

	std::string address;
	if (ac == 2) {
		address = av[1];
	}

	Server* server = new Server();
	// Server server;

	try {
		// server.prepareListeningPort();
		server->prepareListeningPort(address);
	} catch (Server::SocketException& e) {
		std::cerr	<< "\e[31mError: socket: " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 10;
	} catch (Server::BindException& e) {
		std::cerr	<< "\e[31mError: bind: " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 11;
	} catch (Server::ListenException& e) {
		std::cerr	<< "\e[31mError: listen: " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 12;
	} catch (const std::exception& e) {
		std::cerr	<< "\e[31mError: No server created. " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 13;
	}

	try {
		server->prepareEPollInstance();
	} catch (Server::CreateEPollException& e) {
		std::cerr	<< "\e[31mError: epoll_create " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 21;
	} catch (Server::ModifyEPollException& e) {
		std::cerr	<< "\e[31mError: epoll_ctl: " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 22;
	} catch (const std::exception& e) {
		std::cerr	<< "\e[31mError: No epoll instance created. " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 23;
	}

	try {
		server->handleIncomingEvents();
	} catch (Server::EventPollingException& e) {
		std::cerr	<< "\e[31mError: epoll_wait: " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 31;
	} catch (Server::AcceptException& e) {
		std::cerr	<< "\e[31mError: accept: " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 32;
	} catch (Server::GetFlagsException& e) {
		std::cerr	<< "\e[31mError: fcntl(F_GETFL): " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 33;
	} catch (Server::SetFlagsException& e) {
		std::cerr	<< "\e[31mError: fcntl(F_SETFL): " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 34;
	} catch (Server::ModifyEPollException& e) {
		std::cerr	<< "\e[31mError: epoll_ctl: " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 35;
	} catch (Server::ReadDataException& e) {
		std::cerr	<< "\e[31mError: recv: " << e.what()
		// std::cout	<< "\e[31mServer shutdown initiated"
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 36;
	} catch (Server::FlushDataException& e) {
		std::cerr	<< "\e[31mError: send: " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 37;
	} catch (const std::exception& e) {
		std::cerr	<< "\e[31mError: Event handling failed. " << e.what()
					<< "\e[0m" << std::endl;
		server->cleanUpAllRessources();
		delete server;
		return 38;
	}

	server->cleanUpAllRessources();
	delete server;
	return 0;
}
