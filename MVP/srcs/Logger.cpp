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
// #include "../incs/colors.hpp"
// #include <unistd.h>
#include <iostream>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

/*	@brief Instance	*/
Logger& Logger::instance(void) {
	static Logger instance;
	return instance;
}

void Logger::setLevel(LogLevel lvl) {
	_logLevel = lvl;
}

void Logger::setLevel(const std::string& input) {
	// if (name == NULL) return;
	std::string name(input);
	for (std::string::iterator i = name.begin(); i != name.end(); ++i)
		*i = std::toupper(*i);
	if (name == "DEBUG") _logLevel = LOG_LEVEL_DEBUG;
	else if (name == "INFO") _logLevel = LOG_LEVEL_INFO;
	else if (name == "WARNING") _logLevel = LOG_LEVEL_WARNING;
	else if (name == "WARN") _logLevel = LOG_LEVEL_WARNING;
	else if (name == "ERROR") _logLevel = LOG_LEVEL_ERROR;
	else if (name == "OFF") _logLevel = LOG_LEVEL_OFF;
}

LogLevel Logger::getLevel(void) const {
	return _logLevel;
}

const char* Logger::getLevelName(LogLevel lvl) const {
	switch (lvl) {
	case LOG_LEVEL_DEBUG:   return "DEBUG";
	case LOG_LEVEL_INFO:    return "INFO";
	case LOG_LEVEL_WARNING: return "WARN";
	case LOG_LEVEL_ERROR:   return "ERROR";
	default:                return "OFF";
	}
}

const char* Logger::getLevelColor(LogLevel lvl) const {
	switch (lvl) {
	case LOG_LEVEL_DEBUG:   return COLOR_DEBUG;
	case LOG_LEVEL_INFO:    return COLOR_INFO;
	case LOG_LEVEL_WARNING: return COLOR_WARNING;
	case LOG_LEVEL_ERROR:   return COLOR_ERROR;
	default:                return COLOR_RESET;
	}
}

void Logger::debug(const std::string& msg) {
	// std::cout << "DING-D!" << std::endl;
	logMessage(LOG_LEVEL_DEBUG, msg);
}

void Logger::info(const std::string& msg) {
	// std::cout << "DING-I!" << std::endl;
	logMessage(LOG_LEVEL_INFO, msg);
}

void Logger::warning(const std::string& msg) {
	// std::cout << "DING-W!" << std::endl;
	logMessage(LOG_LEVEL_WARNING, msg);
}

void Logger::error(const std::string& msg) {
	// std::cout << "DING-E!" << std::endl;
	logMessage(LOG_LEVEL_ERROR, msg);
}

void Logger::notice(const std::string& msg) {
	// std::cout << "BLUBB!" << std::endl;
	// logMessage(LOG_LEVEL_NOTICE, msg);
	if (LOG_LEVEL_NOTICE < _logLevel) return;
	std::string color = getLevelColor(LOG_LEVEL_NOTICE);
	std::cout << color << msg << COLOR_RESET << std::endl;
}

void Logger::logMessage(LogLevel lvl, const std::string& msg) {
	if (lvl < _logLevel || lvl == LOG_LEVEL_OFF) return;

	// std::cout << "DONG!" << std::endl;
	std::string name = getLevelName(lvl);
	std::string color = getLevelColor(lvl);

	std::ostream* stream = &std::cerr;
	if (lvl == LOG_LEVEL_DEBUG || lvl == LOG_LEVEL_INFO) {
		stream = &std::cout;
	}
	*stream << color << "[" << name << "]\t" << msg << COLOR_RESET << std::endl;
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

/*	@brief Constructor	*/
Logger::Logger(void) : _logLevel(LOG_LEVEL_NOTICE) {
	// std::cerr << DEBUG << "[DEBUG] Logger Constructor called" << RESET << std::endl;
	return;
}

/*	@brief Destructor	*/
Logger::~Logger(void) {
	// std::cerr << DEBUG << "[DEBUG] Logger Deconstructor called" << RESET << std::endl;
	return;
}

/*	@brief Copy Constructor	*/
Logger::Logger(const Logger& other) {
	*this = other;
	return;
}

/*	@brief Copy Assignment Operator	*/
Logger& Logger::operator = (const Logger& other) {
	if (this != &other) {}
	return *this;
}

// if (lvl == LOG_LEVEL_INFO) {
// 	std::cout << color << "[" << name << "] ";
// 	for (size_t i = 0; i < msg.size(); ++i) {
// 		std::cout << msg[i];
// 	}
// 	std::cout << COLOR_RESET << std::endl;
// } else {
// 	std::cerr << color << "[" << name << "] ";
// 	for (size_t i = 0; i < msg.size(); ++i) {
// 		std::cerr << msg[i];
// 	}
// 	std::cerr << COLOR_RESET << std::endl;
// }
// std::ostream* stream = &std::cerr;
// if (lvl == LOG_LEVEL_INFO) {
// 	stream = &std::cout;
// }
// *stream << color << "[" << name << "] ";
// for (size_t i = 0; i < msg.size(); ++i) {
// 	*stream << msg[i];
// }
// *stream << COLOR_RESET << std::endl;
