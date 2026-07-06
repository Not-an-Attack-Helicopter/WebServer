/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   templates.tpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/03 11:21:59 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/03 11:22:00 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TEMPLATES_TPP
#define TEMPLATES_TPP

#include <string>
#include <sstream>

template<typename T>
std::string i2a(T input) {
	std::stringstream convert;
	convert << input;
	return convert.str();
}

template<typename T, int N>
int arraySize(T (&)[N]) {
	return N;
}

#endif
