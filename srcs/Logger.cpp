/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 15:07:15 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/15 15:07:17 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/Logger.hpp"
#include <iostream>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Instance	*/
Logger& Logger::instance(void) {
	static Logger instance;
	return instance;
}

void Logger::setLevel(Level lvl) {
	_log_level = lvl;
	return;
}

void Logger::setLevel(const std::string& input) {
	if (input.empty()) return;
	std::string name(input);
	for (std::string::iterator i = name.begin(); i != name.end(); ++i)
		*i = std::toupper(*i);
	if (name == "DEBUG") _log_level = LEVEL_DEBUG;
	else if (name == "INFO") _log_level = LEVEL_INFO;
	else if (name == "WARNING") _log_level = LEVEL_WARNING;
	else if (name == "WARN") _log_level = LEVEL_WARNING;
	else if (name == "ERROR") _log_level = LEVEL_ERROR;
	else if (name == "OFF") _log_level = LEVEL_OFF;
	return;
}

Logger::Level Logger::getLevel(void) const {
	return _log_level;
}

const char* Logger::getLevelName(Level lvl) const {
	switch (lvl) {
	case LEVEL_DEBUG:   return "DEBUG";
	case LEVEL_INFO:    return "INFO";
	case LEVEL_WARNING: return "WARN";
	case LEVEL_ERROR:   return "ERROR";
	default:                return "OFF";
	}
}

const char* Logger::getLevelColor(Level lvl) const {
	switch (lvl) {
	case LEVEL_DEBUG:   return COLOR_DEBUG;
	case LEVEL_INFO:    return COLOR_INFO;
	case LEVEL_WARNING: return COLOR_WARNING;
	case LEVEL_ERROR:   return COLOR_ERROR;
	default:                return COLOR_RESET;
	}
}

void Logger::debug(const std::string& msg) {
	_logMessage(LEVEL_DEBUG, msg);
	return;
}

void Logger::info(const std::string& msg) {
	_logMessage(LEVEL_INFO, msg);
	return;
}

void Logger::warn(const std::string& msg) {
	_logMessage(LEVEL_WARNING, msg);
	return;
}

void Logger::error(const std::string& msg) {
	_logMessage(LEVEL_ERROR, msg);
	return;
}

void Logger::notice(const std::string& msg) {
	if (LEVEL_NOTICE < _log_level) return;
	std::string color = getLevelColor(LEVEL_NOTICE);
	std::cout << color << msg << COLOR_RESET << std::endl;
	return;
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Constructor	*/
Logger::Logger(void) : _log_level(LEVEL_OFF) {
	// std::cerr << DEBUG << "[DEBUG] Logger Constructor called" << RESET << std::endl;
	return;
}

/*	@brief Destructor	*/
Logger::~Logger(void) {
	// std::cerr << DEBUG << "[DEBUG] Logger Destructor called" << RESET << std::endl;
	return;
}

/*	@brief Copy Constructor	*/
Logger::Logger(const Logger& other) {
	// std::cerr << DEBUG << "[DEBUG] Logger Destructor called" << RESET << std::endl;
	*this = other;
	return;
}

/*	@brief Copy Assignment Operator	*/
Logger& Logger::operator = (const Logger& other) {
	if (this != &other) {
		std::cerr << COLOR_DEBUG << "[DEBUG] Logger Copy Assignment Operator called" << COLOR_RESET << std::endl;
	}
	return *this;
}

void Logger::_logMessage(Level lvl, const std::string& msg) {
	if (lvl < _log_level || lvl == LEVEL_OFF) return;

	std::string name = getLevelName(lvl);
	std::string color = getLevelColor(lvl);

	std::ostream* stream = &std::cerr;
	if (lvl == LEVEL_DEBUG || lvl == LEVEL_INFO) {
		stream = &std::cout;
	}
	*stream << color << "[" << name << "]\t" << msg << COLOR_RESET << std::endl;
	return;
}


// if (lvl == LEVEL_INFO) {
// 	std::cout << color << "[" << name << "] ";
// 	for (std::size_t i = 0; i < msg.size(); ++i) {
// 		std::cout << msg[i];
// 	}
// 	std::cout << COLOR_RESET << std::endl;
// } else {
// 	std::cerr << color << "[" << name << "] ";
// 	for (std::size_t i = 0; i < msg.size(); ++i) {
// 		std::cerr << msg[i];
// 	}
// 	std::cerr << COLOR_RESET << std::endl;
// }
// std::ostream* stream = &std::cerr;
// if (lvl == LEVEL_INFO) {
// 	stream = &std::cout;
// }
// *stream << color << "[" << name << "] ";
// for (std::size_t i = 0; i < msg.size(); ++i) {
// 	*stream << msg[i];
// }
// *stream << COLOR_RESET << std::endl;
