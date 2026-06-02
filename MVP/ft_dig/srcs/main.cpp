/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 17:48:02 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/02 20:29:15 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// #include <netinet/in.h>
#include <arpa/inet.h>
// #include <sys/socket.h>
// #include <netdb.h>
#include <iostream>
#include "../incs/AddressInfo.hpp"

int main(int ac, char** av) {

	if (ac == 1) {

		char	buffer[INET_ADDRSTRLEN];
		int		len = sizeof(buffer);

		// struct sockaddr_in lb;
		// inet_pton(AF_INET, "127.0.0.1", &lb.sin_addr);
		std::cout << "\n127.0.0.1:" << std::endl;
		std::cout << "----------" << std::endl;
		std::cout << "INADDR_LOOPBACK (host order)\t\t" << INADDR_LOOPBACK << std::endl;
		// std::cout << "lb.sin_addr.s_addr (host order)\t\t" << ntohl(lb.sin_addr.s_addr) << std::endl;
		std::cout << "INADDR_LOOPBACK (network order)\t\t" << htonl(INADDR_LOOPBACK) << std::endl;
		// std::cout << "lb.sin_addr.s_addr (network order)\t" << lb.sin_addr.s_addr << std::endl;

		struct in_addr ia;
		inet_pton(AF_INET, "216.58.192.3", &ia);
		std::cout << "\n216.58.192.3:" << std::endl;
		std::cout << "-------------" << std::endl;
		std::cout << "ia.s_addr (host order)\t\t\t" << ntohl(ia.s_addr) << std::endl;
		std::cout << "ia.s_addr (network order)\t\t" << ia.s_addr << std::endl;

		struct sockaddr_in sa;
		inet_pton(AF_INET, "216.58.192.3", &sa.sin_addr);
		inet_ntop(AF_INET, &(sa.sin_addr), buffer, len);
		std::cout << "\n" << buffer << ":"  << std::endl;
		std::cout << "-------------" << std::endl;
		std::cout << "sa.sin_addr.s_addr (host order)\t\t" << ntohl(sa.sin_addr.s_addr) << std::endl;
		std::cout << "sa.sin_addr.s_addr (network order)\t" << sa.sin_addr.s_addr << std::endl;

		std::cout << "\ntry: " << av[0] << " <hostname>" << std::endl;

		return (0);

	} else if (ac == 2) {

		try {
			AddressInfo::getInstance(av[1]).gai();
		} catch (const std::exception& e) {
			std::cerr << "\e[31mError: " << e.what() << "\e[0m" << std::endl;
		}

		// delete ai;

		return (0);

	} else {

		std::cerr << "usage: " << av[0] << " <hostname>" << std::endl;
		return (1);

	}

}

// struct addrinfo		req = {};
// struct addrinfo*	pai = 0;
// struct addrinfo*	head = 0;
// int					status;
// char				buff[INET_ADDRSTRLEN];
// int					len = sizeof(buff);
// char				buff6[INET6_ADDRSTRLEN];
// int					len6 = sizeof(buff6);

// req.ai_family = AF_UNSPEC;
// req.ai_socktype = SOCK_STREAM;

// status = getaddrinfo(av[1], 0, &req, &pai);
// if (status) {
// 	std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
// 	return (2);
// }

// std::cout << "IP addresses for " << av[1] << ":" << std::endl;

// head = pai;

// while (head) {

// 	if (head->ai_family == AF_INET) {

// 		struct sockaddr_in* ipv4 = (struct sockaddr_in*) head->ai_addr;
// 		inet_ntop(AF_INET, &(ipv4->sin_addr), buff, len);
// 		std::cout << "IPv4: " << buff << std::endl;

// 	} else {

// 		struct sockaddr_in6* ipv6 = (struct sockaddr_in6*) head->ai_addr;
// 		inet_ntop(AF_INET6, &(ipv6->sin6_addr), buff6, len6);
// 		std::cout << "IPv6: " << buff6 << std::endl;

// 	}

// 	head = head->ai_next;

// }

// freeaddrinfo(pai);

// AddressInfo ai(av[1]);
// AddressInfo* ai = new AddressInfo(av[1]);
