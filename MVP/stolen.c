// server.c - a micro-server that accepts a connection before quitting
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
// #include <sys/types.h>

#define PORT 4242 // our server's port
#define BACKLOG 100  // max number of connection requests in queue

int main(void)
{
	struct sockaddr_in sa;
	int socket_fd = 0;
	int client_fd = 0;
	int status;
	// sockaddr_storage is a structure that is not associated to
	// a particular family. This allows us to receive either
	// an IPv4 or an IPv6 address
	struct sockaddr_storage client_addr;
	socklen_t addr_size;

	// Prepare the address and port for the server socket
	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET; // IPv4 only; use AF_INET6 for IPv6
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1, localhost
	sa.sin_port = htons(PORT);

	// create socket, bind it and listen with it
	socket_fd = socket(sa.sin_family, SOCK_STREAM, 0);
	int flags = fcntl(socket_fd, F_GETFL);
	fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK | SO_REUSEADDR);
	status = bind(socket_fd, (struct sockaddr *)&sa, sizeof sa);
	if (status != 0) {
		fprintf(stderr, "bind: %s\n", strerror(errno));
		return (2);
	}
	listen(socket_fd, BACKLOG);

	// Accept incoming connection
	addr_size = sizeof client_addr;
	client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &addr_size);
	if (client_fd == -1) {
		fprintf(stderr, "accept: %s\n", strerror(errno));
		return (3);
	}
	// flags = fcntl(socket_fd, F_GETFL);
	// fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK | SO_REUSEADDR);
	printf("New connection! Socket fd: %d, client fd: %d\n", socket_fd, client_fd);

	// We are ready to communicate with the client via the client_fd!

	close(client_fd);
	close(socket_fd);

	return (0);
}
