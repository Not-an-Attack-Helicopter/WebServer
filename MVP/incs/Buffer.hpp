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

static const size_t BUFFER_SIZE = 4 * 1024;

struct Buffer {

	std::vector<char> data;
	size_t begin;
	size_t mark;
	size_t end;

	// std::vector<char>::const_iterator begin_it = data.begin() + begin;
	// std::vector<char>::const_iterator end_it = data.begin() + end;

	std::string str(void) const;
	std::string substr(ssize_t begin) const;
	std::string substr(ssize_t begin, ssize_t end) const;

	void sstream(std::stringstream& ss) const;
	void sstream(std::stringstream& ss, ssize_t begin) const;
	void sstream(std::stringstream& ss, ssize_t begin, ssize_t end) const;

	void reset(void);
	void compact(void);

	size_t range(void) const;

	ssize_t find(const char& pin) const;
	ssize_t find(const std::string& needle) const;

	Buffer(void) : begin(0), mark(0), end(0) {data.resize(BUFFER_SIZE);}

};

#endif
