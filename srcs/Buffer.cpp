/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Buffer.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/25 14:51:14 by bstorck           #+#    #+#             */
/*   Updated: 2026/08/25 14:51:15 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/Buffer.hpp"
#include "../incs/Logger.hpp"
#include <sys/socket.h>
#include <algorithm>
#include <cstring>
// #include <cstddef>

std::string Buffer::str(void) const {
	return std::string(data.begin() + begin, data.begin() + end);
}

std::string Buffer::substr(size_t offset) const {
	return std::string(data.begin() + begin + offset, data.begin() + end);
}

std::string Buffer::substr(size_t offset1, std::size_t offset2) const {
	return std::string(data.begin() + begin + offset1, data.begin() + begin + offset2);
}

void Buffer::sstream(std::stringstream& ss) const {
	ss.write(&data[begin], range());
}

void Buffer::sstream(std::stringstream& ss, std::size_t offset) const {
	ss.write(&data[begin + offset], end - (begin + offset));
}

void Buffer::sstream(std::stringstream& ss, std::size_t offset1, std::size_t offset2) const {
	ss.write(&data[begin + offset1], offset2 - offset1);
}

void Buffer::reset(void) {
	end = 0;
	mark = 0;
	begin = 0;
}

void Buffer::compact(void) {
	log.error("buffer is compacted");
	std::memmove(&data[0], &data[begin], range());
	mark -= begin;
	end -= begin;
	begin = 0;
}

std::size_t Buffer::range(void) const {
	return end - begin;
}

ssize_t Buffer::find(const char& pin) const {
	std::vector<char>::const_iterator begin_it = data.begin() + begin;
	std::vector<char>::const_iterator end_it = data.begin() + end;
	std::vector<char>::const_iterator it = std::find(begin_it,
													 end_it,
													 pin);
	return (it != end_it) ? std::distance(begin_it, it) : -1;
}

ssize_t Buffer::find(const std::string& needle) const {
	std::vector<char>::const_iterator end_it = data.begin() + end;
	std::vector<char>::const_iterator begin_it = data.begin() + begin;
	std::vector<char>::const_iterator it = std::search(begin_it, end_it,
													   needle.begin(), needle.end());
	return (it != end_it) ? std::distance(begin_it, it) : -1;
}

ssize_t Buffer::fetchData(int fd) {

	ssize_t n = 0;
	if (end < data.size()) {
		n = recv(fd, &data[end], data.size() - end, 0);
		if (n <= 0) return n;
		end += static_cast<std::size_t>(n);
	}
	return n;
}

ssize_t Buffer::flushData(int fd) {

	ssize_t n = 0;
	if (begin < end) {
		n = send(fd, &data[begin], end - begin, 0);
		if (n <= 0) return n;
		begin += static_cast<std::size_t>(n);
	}
	return n;
}
