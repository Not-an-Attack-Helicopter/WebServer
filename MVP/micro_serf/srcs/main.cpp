#include "../incs/Server.hpp"
// #include "../incs/Client.hpp"
#include <fcntl.h>
#include <iostream>

void setNonblockFlag(int fd) {
	int flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(void) {

	try {
		// Server* microSerf = new Server();
		Server microSerf(void);
	} catch  (const std::exception& e) {
		std::cerr	<< "\e[31mError: No server created. "
					<< e.what() << "\e[0m"
					<< std::endl;
	}

	return (0);
}
