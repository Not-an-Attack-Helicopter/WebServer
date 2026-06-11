#include "webserver.hpp"


int main(int argc, char **argv)
{

	std::cout << "Starting WebServer..." << std::endl;
	// Server server;
	if (argc >= 3 && std::string(argv[1]) == "-v")
	{
		std::cout << "Using: " << argv[2] << std::endl;
		try
		{
			ConfigParser parser(argv[2]);
			print_conf(parser.get_config());
		}
		catch (const std::exception& e)
		{
			std::cerr	<< "\e[31mError: Failed to parse config file: " << e.what()
						<< "\e[0m" << std::endl;
		}

	}
	else if (argc >= 2)
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
