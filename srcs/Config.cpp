/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/31 11:50:57 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/31 11:50:59 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/Config.hpp"
#include "../incs/Logger.hpp"

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Instance	*/
Config& Config::instance(void) {
	static Config instance;
	return instance;
}

const std::vector<Config::Socket>& Config::get() const {
	return _configs;
}

const Config::Socket& Config::get(std::size_t index) const {
	return _configs[index];
}

std::size_t Config::size(void) const {
	return _configs.size();
}

void Config::pushConfig(Socket socket) {
	_configs.push_back(socket);
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Constructor	*/
Config::Config(void) {
	log.debug("Config Constructor called");
	return;
};

/*	@brief Copy Constructor	*/
Config::Config(const Config& other) : _configs(other._configs) {
	log.debug("Config Copy Constructor called");
	return;
};

/*	@brief Copy Assignment Operator	*/
Config& Config::operator=(const Config& other) {
	log.debug("Config Copy Assignment Operator called");
	if (this != &other) {
		this->_configs = other._configs;
	}
	return *this;
};

/*	@brief Destructor	*/
Config::~Config() {
	log.debug("Config Destructor called");
	return;
};
