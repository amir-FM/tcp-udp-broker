#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include "helper.h"

#define DEBUG 1
#define MAX_CONNECTIONS 10

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

		rc = listen(tcpfd, MAX_CONNECTIONS);
		DIE(rc < 0, "listen");

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

class TCP_Connect{
public:
	TCP_Connect(){};

	TCP_Connect(int listenfd){
		this->listenfd = listenfd;
	}

	int new_connection(){
		struct sockaddr_in cli_addr;
		socklen_t cli_len = sizeof(cli_addr);

		newsockfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
		DIE(newsockfd < 0, "accept");

		cout << "New connection from " << inet_ntoa(cli_addr.sin_addr) << " at " << ntohs(cli_addr.sin_port) << "\n";

		return newsockfd;
	}

	void close_connection(){
		close(newsockfd);
		if(DEBUG)cout << "closed: " << newsockfd << endl;
	}

	void close_connection(int fd){
		close(fd);
		if(DEBUG)cout << "closed: " << fd << endl;
	}

	void recv_and_print(){
		uint8_t packet[1500];

		int rc = recv(newsockfd, &packet, sizeof(packet), 0);
		DIE(rc < 0, "recv");

		if(rc)
			cout << packet << "\n---------------\n\n";
	}

	int recv_and_back(){
		int rc;
		uint8_t packet[1500];

		rc = recv(newsockfd, &packet, sizeof(packet), 0);
		DIE(rc < 0, "recv");

		if(rc == 0){
			close_connection();
			return -2;
		}

		rc = send(newsockfd, &packet, rc, 0);
		DIE(rc < 0, "send");
		return 0;
	}

	int recv_and_back(int fd){
		int rc;
		uint8_t packet[1500];

		rc = recv(fd, &packet, sizeof(packet), 0);
		DIE(rc < 0, "recv");

		if(rc == 0){
			close_connection(fd);
			return -2;
		}

		rc = send(fd, &packet, rc, 0);
		DIE(rc < 0, "send");
		return 0;
	}

	int get_listenfd(){
		return listenfd;
	}
	
private:
	int listenfd;
	int newsockfd;

};

class Multiplexer{
public:
	Multiplexer(TCP_Connect t){
		this->t = t;
		this->listenfd = t.get_listenfd();
		add_fd(this->listenfd);

		num_sockets = 1;
	}

	void poll_wait(){
		int rc = poll(poll_fds.data(), num_sockets, -1);
		DIE(rc < 0, "poll");
	}

	void add_fd(int fd){
		struct pollfd p = {.fd = fd, .events = POLLIN};
		poll_fds.push_back(p);
		num_sockets++;
		if(DEBUG)cout << "added: " << fd << endl;
	}

	void remove_fd(int index){
		int fd = poll_fds[index].fd;
		poll_fds.erase(poll_fds.begin() + index);
		if(DEBUG)cout << "Removed: " << fd << endl;
	}

	void check_events(){
		for(int i = 0; i < num_sockets; i++){
			if(poll_fds[i].revents & POLLIN){
				if(poll_fds[i].fd == listenfd){
					add_fd(t.new_connection());
				}else{
					int rc = t.recv_and_back(poll_fds[i].fd);
					if(rc == -2)remove_fd(i);
				}
				break;
			}
		}
	}

private:
	TCP_Connect t;
	vector<struct pollfd> poll_fds;
	int listenfd;
	int num_sockets;
};

int main(int argc, char *argv[]){
	Server s(argv[1]);
	s.start();

	UDP_Connect u(s.get_udp_fd());
	TCP_Connect t(s.get_tcp_fd());
	Multiplexer x(t);

	while(1){
		x.poll_wait();
		x.check_events();
	}
}
