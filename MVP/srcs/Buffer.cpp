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
#include <algorithm>
#include <cstring>
#include <cstddef>
// #include <fcntl.h>

std::string Buffer::str(void) const {
	return std::string(data.begin() + begin, data.begin() + end);
}

std::string Buffer::substr(ssize_t offset) const {
	return std::string(data.begin() + begin + offset, data.begin() + end);
}

std::string Buffer::substr(ssize_t offset1, ssize_t offset2) const {
	return std::string(data.begin() + begin + offset1, data.begin() + begin + offset2);
}

void Buffer::sstream(std::stringstream& ss) const {
	ss.write(&data[begin], range());
}

void Buffer::sstream(std::stringstream& ss, ssize_t offset) const {
	ss.write(&data[begin + offset], end - (begin + offset));
}

void Buffer::sstream(std::stringstream& ss, ssize_t offset1, ssize_t offset2) const {
	ss.write(&data[begin + offset1], offset2 - offset1);
}

void Buffer::reset(void) {
	end = 0;
	mark = 0;
	begin = 0;
	// data.clear();
	// data.resize(BUFFER_SIZE);
}

void Buffer::compact(void) {
	std::memmove(&data[0], &data[begin], range());
	mark -= begin;
	end -= begin;
	begin = 0;
}

size_t Buffer::range(void) const {
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
													   needle.begin(), needle.end()
													   // , std::equal_to<char>()
													   );
	return (it != end_it) ? std::distance(begin_it, it) : -1;
}

