/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   templates.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/03 11:20:31 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/03 11:20:32 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TEMPLATES_HPP
#define TEMPLATES_HPP

#include <cstddef>
#include <string>
#include <sstream>
#include <vector>

template<typename T>
inline std::string i2a(const T input) {
	std::stringstream convert;
	convert << input;
	return convert.str();
}

template<typename T, size_t N>
inline size_t arraySize(const T (&)[N]) {
	return N;
}

template<typename T>
inline typename std::vector<T>::iterator pos2it(std::vector<T>& vec, size_t pos) {
	return vec.begin() + pos;
}

template<typename T>
inline typename std::vector<T>::const_iterator pos2it(const std::vector<T>& vec, size_t pos) {
	return vec.begin() + pos;
}

#endif
