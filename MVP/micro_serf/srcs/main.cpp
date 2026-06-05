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

	try {
		// Server* microSerf = new Server();
		Server microSerf(void);
	} catch (Server::SocketException& e) {
		std::cerr	<< "\e[31mError: socket: " << e.what() << "\e[0m"
					<< std::endl;
	} catch (Server::GetFlagsException& e) {
		std::cerr	<< "\e[31mError: fcntl(F_GETFL): " << e.what() << "\e[0m"
					<< std::endl;
	} catch (Server::SetFlagsException& e) {
		std::cerr	<< "\e[31mError: fcntl(F_SETFL): " << e.what() << "\e[0m"
					<< std::endl;
	} catch (const std::exception& e) {
		std::cerr	<< "\e[31mError: No server created. " << e.what() << "\e[0m"
					<< std::endl;
	}

	return 0;
}
