#include <iostream>
#include <cstdint>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "helper.h"

#define DEBUG 1

using namespace std;

class Server{
public:
	uint16_t port;

	Server(uint16_t port){
		this->port = port;
	}

	void start(){
		int rc;

		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		DIE(listenfd < 0, "socket");

		int enable = 1;
		rc = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
		DIE(rc < 0, "setsockopt");

		set_serv_addr();

		rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));

		DIE(rc < 0, "bind");
		if(DEBUG)cout << "Started TCP server\n";
	}

	void stop(){
		close(listenfd);
	}

private:
	int listenfd;
	struct sockaddr_in serv_addr;

	void set_serv_addr(){
		socklen_t socket_len = sizeof(struct sockaddr_in);

		memset(&serv_addr, 0, socket_len);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
		serv_addr.sin_addr.s_addr = INADDR_ANY;
	}
};

int main(int argc, char *argv[]){
	Server s(8080);
	s.start();

	while(1){};
}
