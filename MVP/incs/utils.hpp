/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sholz + bstorck <marvin@42.fr>             +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 18:39:31 by sholz             #+#    #+#             */
/*   Updated: 2026/06/30 18:39:32 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

// #include "webserver.hpp"
// #include "types.hpp"
#include "Client.hpp"
#include "HTTPRequest.hpp"
#include <sys/epoll.h>
#include <string>
// #include <vector>

void			dumpConfigs(const std::vector<Config>& config);
// DEBUG
void			dumpClientConfig(const Client* client);
void			dumpEvents(int nfds, epoll_event* events);
void			dumpRequest(HTTPRequest* request);

void			warnHighEventLoad(int nfds, int max_capacity);
// DEBUG
unsigned short	stringToUnsignedShort(const std::string& str);
size_t			stringToSize(const std::string& str);
int				stringToInt(const std::string& str);

std::string		trim(const std::string& str);
std::string		get_content_type(const std::string& path);

#endif
