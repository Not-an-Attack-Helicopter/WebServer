/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 15:05:16 by bstorck           #+#    #+#             */
/*   Updated: 2026/06/15 15:05:19 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

// Logging shorthand — pragmatic exception to macro avoidance
#define log Logger::instance()

#define COLOR_DEBUG "\e[3;94m"
#define COLOR_INFO "\e[93m"
#define COLOR_WARNING "\e[33m"
#define COLOR_ERROR "\e[31m"
#define COLOR_NOTICE "\x1b[97m"
#define COLOR_RESET "\e[0m"

class Logger {

public:
	static Logger&		instance(void);

	enum Level {
		LEVEL_DEBUG,
		LEVEL_INFO,
		LEVEL_WARNING,
		LEVEL_ERROR,
		LEVEL_NOTICE,
		LEVEL_OFF
	};

	void				setLevel(Level lvl);
	void				setLevel(const std::string& name);

	Level				getLevel(void) const;

	const char*			getLevelName(Level lvl) const;
	const char*			getLevelColor(Level lvl) const;

	void				debug(const std::string& msg);
	void				info(const std::string& msg);
	void				warn(const std::string& msg);
	void				error(const std::string& msg);
	void				notice(const std::string& msg);

private:
	Logger(void);
	~Logger(void);
	Logger(const Logger&);
	Logger& operator=(const Logger&);

	void				_logMessage(Level lvl, const std::string& msg);

	Level				_log_level;

};

#endif
