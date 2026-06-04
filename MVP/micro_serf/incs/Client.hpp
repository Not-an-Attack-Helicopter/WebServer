#ifndef CLIENT_H
# define CLIENT_H

# include <string>
#include <sys/socket.h>

class Client {

	public:
		Client(void);
		~Client(void);
		Client(const Client& other);
		Client& operator = (const Client& other);

		const std::string&			getReadBuffer(void) const;
		const std::string&			getWriteBuffer(void) const;

		// void						setReadBuffer(void);
		// void						setWriteBuffer(const std::string& data);

		bool						hasPendingWrites(void) const;

		void						queueResponse(const std::string& message);

		size_t						flush(int fd);

		struct sockaddr_storage		sa;

		socklen_t					addrLen;

		// int							sockfd;

	private:
		std::string					_readBuffer;
		std::string					_writeBuffer;
};

#endif

// struct sockaddr*			getSockAddr(void) const;
// int							getFildes(void) const;
// void						setSockAddr(void);
// void						setFildes(void);
