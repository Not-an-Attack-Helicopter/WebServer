/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Buffer.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 14:46:05 by bstorck           #+#    #+#             */
/*   Updated: 2026/08/25 14:46:07 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vector>
#include <sstream>
#include <cstddef>
#include <sys/types.h>

static const std::size_t BUFFER_SIZE = 4 * 1024;

struct Buffer {

	std::vector<char> data;
	std::size_t begin;
	std::size_t mark;
	std::size_t end;

	std::string str(void) const;
	std::string substr(size_t begin) const;
	std::string substr(size_t begin, std::size_t end) const;

	void sstream(std::stringstream& ss) const;
	void sstream(std::stringstream& ss, std::size_t begin) const;
	void sstream(std::stringstream& ss, std::size_t begin, std::size_t end) const;

	void reset(void);
	void compact(void);

	std::size_t range(void) const;

	ssize_t find(const char& pin) const;
	ssize_t find(const std::string& needle) const;

	// is_pipe picks read()/write() instead of recv()/send() -- recv/send
	// only work on sockets, pipes need the plain syscalls (ENOTSOCK otherwise)
	ssize_t fetchData(int fd, bool is_pipe = false);
	ssize_t flushData(int fd, bool is_pipe = false);

	Buffer(void) : begin(0), mark(0), end(0) {data.resize(BUFFER_SIZE);}

};

#endif
