#include "webserver.hpp"

int main(int argc, char **argv)
{

	std::cout << "Starting WebServer..." << std::endl;
	// webserver server;
	if(argc >= 2)
	{
		std::cout << "Using: " << argv[1] << std::endl;
		//server.start(argv[1]);
	}
	else
	{
		std::cout << "Using Default Configuration" << std::endl;
		//server.start("Config_Files/default.conf");
	}

	return 0;
}