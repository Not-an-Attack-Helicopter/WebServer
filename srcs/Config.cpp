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
// #include "../incs/Dispatcher.hpp"
// #include "../incs/templates.hpp"
#include "../incs/Logger.hpp"
// #include <algorithm>
// #include <stdexcept>

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

const Config::Socket& Config::get(size_t index) const {
	return _configs[index];
}

size_t Config::size(void) const {
	return _configs.size();
}

void Config::pushConfig(Socket socket) {
	_configs.push_back(socket);
}

// void Config::validateRedirectChains(void) {
//
// 	std::vector<Config::Socket>::iterator conf_it = _configs.begin();
// 	while (conf_it != _configs.end()) {
// 		std::vector<Domain>::iterator dom_it = conf_it->domains.begin();
// 		while (dom_it != conf_it->domains.end()) {
// 			std::vector<Location>::iterator loc_it = dom_it->locations.begin();
// 			while (loc_it != dom_it->locations.end()) {
// 				unsigned short redirection_count = 0;
// 				std::vector<std::string> redirects;
// 				Location* loc = &(*loc_it);
// 				std::string redirect = loc->path;
// 				if (redirect != "/") redirect.append("/");
// 				redirects.push_back(redirect);
// 				while (!loc->redirect.empty()) {
// 					if (redirect == loc->redirect) {
// 						throw std::runtime_error("parse error: self-redirect at '" + loc->path + "'");
// 					}
// 					// if (redirects.count(loc->redirect)) {
// 					if (std::find(redirects.begin(), redirects.end(), loc->redirect) != redirects.end()) {
// 						log.error("parse error: circular redirect detected at '" + loc->redirect + "'");
// 						log.error("redirect chain: ");
// 						// for (std::vector<std::string>::iterator it = redirects.begin(); it != redirects.end(); ++it) {
// 							// std::cerr << "\e[31m" << *it << " -> \e[0m";
// 							// log.error(*it);
// 						for (size_t i = 0; i < redirects.size(); ++i) {
// 							log.error(redirects[i]);
// 						}
// 						throw std::runtime_error(loc->redirect + " (LOOP)");
// 					}
// 					++redirection_count;
// 					if (redirection_count > MAX_REDIRECTS) {
// 						log.error("parse error: too many consecutive redirects");
// 						log.error("redirect chain: ");
// 						// for (std::vector<std::string>::iterator it = redirects.begin(); it != redirects.end(); ++it) {
// 							// std::cerr << "\e[31m" << *it << " -> \e[0m";
// 							// log.error(*it);
// 						for (size_t i = 0; i < redirects.size(); ++i) {
// 							log.error(redirects[i]);
// 						}
// 						log.error(loc->redirect);
// 						throw std::runtime_error(i2a(redirects.size()) + "/" + i2a(MAX_REDIRECTS) + " hops");
// 					}
// 					redirects.push_back(loc->redirect);
// 					loc = const_cast<Location*>(Dispatcher::matchLocation(dom_it->locations, loc->redirect));
// 				}
// 				++loc_it;
// 			}
// 			++conf_it;
// 		}
// 	}
// 	// return true;
// }

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

/*	@brief Deconstructor	*/
Config::~Config() {
	log.debug("Config Deconstructor called");
	return;
};
