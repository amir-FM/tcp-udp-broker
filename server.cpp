#include <iostream>
#include <cstdint>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "helper.h"

#define DEBUG 1

using namespace std;

struct UDP_Message{
	uint8_t topic[50];
	uint8_t type;
	uint8_t data[1500];
}; 

class Server{
public:
	uint16_t port;

	Server(string port){
		this->port = stoi(port);
	}

	void start(){
		start_udp();
		start_tcp();
	}

	void stop(){
		close_udp();
		close_tcp();
	}

	int get_tcp_fd(){
		return tcpfd;
	}
	
	int get_udp_fd(){
		return udpfd;
	}

private:
	int udpfd, tcpfd;
	struct sockaddr_in serv_addr;

	void start_tcp(){
		int rc;

		tcpfd = socket(AF_INET, SOCK_STREAM, 0);
		DIE(tcpfd < 0, "socket");

		int enable = 1;
		rc = setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
		DIE(rc < 0, "setsockopt");

		set_serv_addr();

		rc = bind(tcpfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));

		DIE(rc < 0, "bind");
		if(DEBUG)cout << "Started TCP server\n";
	}

	void close_tcp(){
		close(tcpfd);
	}

	void start_udp(){
		int rc;

		udpfd = socket(AF_INET, SOCK_DGRAM, 0);
		DIE(udpfd < 0, "socket");

		int enable = 1;
		rc = setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
		DIE(rc < 0, "setsockopt");

		set_serv_addr();

		rc = bind(udpfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));

		DIE(rc < 0, "bind");
		if(DEBUG)cout << "Started UDP server\n";
	}

	void close_udp(){
		close(udpfd);
	}

	void set_serv_addr(){
		socklen_t socket_len = sizeof(struct sockaddr_in);

		memset(&serv_addr, 0, socket_len);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
		serv_addr.sin_addr.s_addr = INADDR_ANY;
	}
};

class UDP_Connect{
public:
	UDP_Connect(int listenfd){
		this->listenfd = listenfd;
	}

	int recv_and_print(){
		clean_message();

		struct sockaddr_in cl_addr;
		socklen_t clen = sizeof(cl_addr);

		int rc = recvfrom(listenfd, &message, sizeof(struct UDP_Message), 0, (struct sockaddr *)&cl_addr, &clen);
		
		if(rc > 0)
			cout << message.topic << " " << message.type << "\n" << message.data<< "\n------------------------------------\n\n";

		return rc;
	}

private:
	int listenfd;
	struct UDP_Message message;

	void clean_message(){
		memset(&message, 0, sizeof(struct UDP_Message));
	}
};

int main(int argc, char *argv[]){
	Server s(argv[1]);
	s.start();

	UDP_Connect u(s.get_udp_fd());

	while(1){
		u.recv_and_print();
	};
}
