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

#define log Logger::instance()
#define COLOR_DEBUG "\e[3;94m"
#define COLOR_INFO "\e[93m"
#define COLOR_WARNING "\e[33m"
#define COLOR_ERROR "\e[31m"
#define COLOR_NOTICE "\x1b[97m"
#define COLOR_RESET "\e[0m"

enum LogLevel {
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_NOTICE,
	LOG_LEVEL_OFF
};

class Logger {

	public:
		static Logger&		instance();

		void				setLevel(LogLevel lvl);
		void				setLevel(const std::string& name);

		LogLevel			getLevel(void) const;

		const char*			getLevelName(LogLevel lvl) const;
		const char*			getLevelColor(LogLevel lvl) const;

		void				debug(const std::string& msg);
		void				info(const std::string& msg);
		void				warning(const std::string& msg);
		void				error(const std::string& msg);
		void				notice(const std::string& msg);
		void				logMessage(LogLevel lvl, const std::string& msg);

	private:
		Logger(void);
		~Logger(void);
		Logger(const Logger&);
		Logger& operator=(const Logger&);

		LogLevel			_logLevel;

};

#endif
