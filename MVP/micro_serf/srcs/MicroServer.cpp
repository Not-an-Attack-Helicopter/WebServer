/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MicroServer.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 07:17:56 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/04 07:17:57 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/MicroServer.hpp"
#include <cstring>
#include <iostream>

  //~~~~~~~~~//
 /*	Public	*/
//~~~~~~~~~//

/*	@brief Constructor	*/
MicroServer::MicroServer(void) {
	std::cerr << "\e[3;93mMicroServer Constructor called\e[0m" << std::endl;
	// _sa.sin_family = 0;
	// _sa.sin_port = 0;
	// _sa.sin_addr.s_addr = 0;
	// for (int i = 0; i < sizeof(_sa.sin_zero); ++i) {
	// 	_sa.sin_zero[i] = 0;
	// }
	_sa.sin_family = AF_INET;
	_sa.sin_port = htons(PORT);
	_sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	memset(_sa.sin_zero, 0, sizeof(_sa.sin_zero));
	return ;
}
