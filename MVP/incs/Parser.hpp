#ifndef PARSER_HPP
#define PARSER_HPP

// #include "webserver.hpp"
#include "types.hpp"

#define parser Parser::instance()

class Parser {

	public:
		static Parser& instance(void);

		const std::vector<Config>&	getAllConfigs(void) const;

		const Config& 				getConfig(size_t index) const;

		size_t								countConfigs(void) const;

		void								readFile(const std::string& config);

	private:
		Parser(void);
		Parser(const Parser& other);
		Parser& operator=(const Parser& other);
		~Parser(void);

		std::vector<Config>			_serverConfigs;

};

#endif
