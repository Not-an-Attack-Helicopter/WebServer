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

// #include "HTTPRequest.hpp"
#include "Client.hpp"
// #include <string>
// #include <cstddef>
#include <sys/epoll.h>

// DEBUG BEGIN
static const short STOP = -2;
void			warnHighEventLoad(int nfds, int max_capacity);
void			dumpEvents(int nfds, epoll_event* events);
void			dumpClientConfig(const Client* client);
void			dumpRequest(const HTTPRequest* request);
// DEBUG END

unsigned short	stringToUnsignedShort(const std::string& str);
std::size_t		stringToSize(const std::string& str);
int				stringToInt(const std::string& str);

std::string		trim(const std::string& str);
std::string		unquote(const std::string& str);
std::string		randomHexString(std::size_t width);

bool			isRegularFile(const std::string& path);
bool			isDirectory(const std::string& path);

void			createFile(HTTPRequest& request);
void			promoteFile(HTTPRequest& request);

void			dumpConfigs(const std::vector<Config::Socket>& config);

/*
 * ================================================================
 * ASCII helpers
 * ================================================================
 */

std::string		tolowerASCII(const std::string& s);
int				hexDigitValue(char c);
bool			isTChar(char c);
bool			isHexDigit(char c);
bool			equalCI(const std::string& a, const std::string& b);

#endif
