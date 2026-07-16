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

// DEBUG
void			warnHighEventLoad(int nfds, int max_capacity);
void			dumpEvents(int nfds, epoll_event* events);
void			dumpClientConfig(const Client* client);
void			dumpRequest(HTTPRequest* request);
// DEBUG

unsigned short	stringToUnsignedShort(const std::string& str);
size_t			stringToSize(const std::string& str);
int				stringToInt(const std::string& str);

std::string		trim(const std::string& str);

bool			isRegularFile(const std::string& path);
bool			isDirectory(const std::string& path);
// bool			isReadable(const std::string& path);
// bool			isValidErrorCode(const int code);

void			dumpConfigs(const std::vector<Config>& config);

#endif
