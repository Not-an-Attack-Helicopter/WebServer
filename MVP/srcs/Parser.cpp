#include "../incs/Parser.hpp"
#include "../incs/parsing.hpp"
#include "../incs/Logger.hpp"
#include <cstddef>

  //~~~~~~~~~~//
 /*  Public  */
//~~~~~~~~~~//

//-------------------------------instance-------------------------------//

/*	@brief Instance	*/
Parser& Parser::instance(void) {
	static Parser instance;
	return instance;
}

//-------------------------------getters-------------------------------//

const std::vector<Config>& Parser::getAllConfigs() const {
	return _serverConfigs;
}

const Config& Parser::getConfig(size_t index) const {
	return _serverConfigs[index];
}

size_t Parser::countConfigs(void) const {
	return _serverConfigs.size();
}

void Parser::readFile(const std::string& config) {
	this->_serverConfigs = parse_config_file(config);
}

  //~~~~~~~~~~~//
 /*  Private  */
//~~~~~~~~~~~//

//-------------------------------Constructors_&_Destructor-------------------------------//

/*	@brief Constructor	*/
Parser::Parser(void) {
	log.debug("Parser Constructor called");
	return;
};

/*	@brief Copy Constructor	*/
Parser::Parser(const Parser& other) : _serverConfigs(other._serverConfigs) {
	log.debug("Parser Copy Constructor called");
	return;
};

/*	@brief Copy Assignment Operator	*/
Parser& Parser::operator=(const Parser& other) {
		log.debug("Parser Copy Assignment Operator called");
	if (this != &other) {
		this->_serverConfigs = other._serverConfigs;
	}
	return *this;
};

/*	@brief Destructor	*/
Parser::~Parser() {
	log.debug("Parser Destructor called");
	return;
};
