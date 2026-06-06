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

void setNonblockFlag(int fd) {
	int flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(void) {

	// Server* microServ = new Server();
	Server microServ;

	try {
		microServ.prepareListeningPort();
	} catch (Server::SocketException& e) {
		std::cerr	<< "\e[31mError: socket: " << e.what()
					<< "\e[0m" << std::endl;
		return 1;
	} catch (Server::BindException& e) {
		std::cerr	<< "\e[31mError: bind: " << e.what()
					<< "\e[0m" << std::endl;
		return 1;
	} catch (Server::ListenException& e) {
		std::cerr	<< "\e[31mError: listen: " << e.what()
					<< "\e[0m" << std::endl;
		return 1;
	} catch (const std::exception& e) {
		std::cerr	<< "\e[31mError: No server created. " << e.what()
					<< "\e[0m" << std::endl;
		return 1;
	}

	try {
		microServ.prepareEPollInstance();
	} catch (Server::CreateEPollException& e) {
		std::cerr	<< "\e[31mError: epoll_create " << e.what()
					<< "\e[0m" << std::endl;
		return 1;
	} catch (Server::ModifyEPollException& e) {
		std::cerr	<< "\e[31mError: epoll_ctl: " << e.what()
					<< "\e[0m" << std::endl;
		return 1;
	} catch (const std::exception& e) {
		std::cerr	<< "\e[31mError: No epoll instance created. " << e.what()
					<< "\e[0m" << std::endl;
		return 1;
	}

	try {
		microServ.handleIncomingEvents();
	} catch (Server::EventPollingException& e) {
		std::cerr	<< "\e[31mError: epoll_wait: " << e.what()
					<< "\e[0m" << std::endl;
		return 1;
	} catch (Server::AcceptException& e) {
		std::cerr	<< "\e[31mError: accept: " << e.what()
		<< "\e[0m" << std::endl;
		return 1;
	} catch (Server::GetFlagsException& e) {
		std::cerr	<< "\e[31mError: fcntl(F_GETFL): " << e.what()
					<< "\e[0m" << std::endl;
		return 1;
	} catch (Server::SetFlagsException& e) {
		std::cerr	<< "\e[31mError: fcntl(F_SETFL): " << e.what()
					<< "\e[0m" << std::endl;
		return 1;
	} catch (Server::ModifyEPollException& e) {
		std::cerr	<< "\e[31mError: epoll_ctl: " << e.what()
					<< "\e[0m" << std::endl;
		return 1;
	} catch (Server::ReadDataException& e) {
		// std::cerr	<< "\e[31mError: recv: " << e.what()
		std::cerr	<< "\e[31mServer shutdown initiated"
					<< "\e[0m" << std::endl;
		return 1;
	} catch (Server::FlushDataException& e) {
		std::cerr	<< "\e[31mError: send: " << e.what()
					<< "\e[0m" << std::endl;
		return 1;
	} catch (const std::exception& e) {
		std::cerr	<< "\e[31mError: Event handling failed. " << e.what()
					<< "\e[0m" << std::endl;
		return 1;
	}

	// delete microServ;
	return 0;
}

// catch (Server::GetFlagsException& e) {
// 	std::cerr	<< "\e[31mError: fcntl(F_GETFL): " << e.what()
// 				<< "\e[0m" << std::endl;
// } catch (Server::SetFlagsException& e) {
// 	std::cerr	<< "\e[31mError: fcntl(F_SETFL): " << e.what()
// 				<< "\e[0m" << std::endl;
// }
