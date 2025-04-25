#include <iostream>
#include <vector>
#include <cmath>
#include <set>
#include <map>
#include <cstdint>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include "helper.h"
#include "protocols.h"

using namespace std;

#define DEBUG 1
#define STDIN 0

class Client{
public:
	Client(string id, string server_ip, string port){
		this->id = id;
		this->ip = server_ip;
		this->port = stoi(port);
	}

	void start(){
		start_tcp();
		login();
	}

	void stop(){
		close_tcp();
	}

	int get_fd(){
		return tcpfd;
	}
	
private:
	int tcpfd;
	struct sockaddr_in serv_addr;
	string id, ip;
	uint16_t port;

	void start_tcp(){
		int rc;

		tcpfd = socket(AF_INET, SOCK_STREAM, 0);
		DIE(tcpfd < 0, "socket");

		set_serv_addr();

		rc = connect(tcpfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
		DIE(rc < 0, "connect");

		if(DEBUG)cout << "Started TCP server\n";
	}

	void close_tcp(){
		close(tcpfd);
	}

	void set_serv_addr(){
		socklen_t socket_len = sizeof(struct sockaddr_in);

		memset(&serv_addr, 0, socket_len);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);

		int rc = inet_pton(AF_INET, (char*)&ip, &serv_addr.sin_addr.s_addr);
		DIE(rc < 0, "inet_pton");
	}

	void login(){
		struct subscribe_message message = {.flag = 2};
		strncpy((char *)message.clid, id.data(), id.size());
		int rc = send(tcpfd, &message, sizeof(message), 0);
		DIE(rc < 0, "send");
	}
};

class TCP_Connect{
public:
	TCP_Connect(){};

	TCP_Connect(int listenfd){
		this->listenfd = listenfd;
	}

	void recv_message(){
		int rc = recv(listenfd, &message, sizeof(struct topic_message), 0);
		DIE(rc < 0, "recv");
	}

	int get_listenfd(){
		return listenfd;
	}
	
	struct topic_message get_message(){
		return message;
	}
	
private:
	int listenfd;
	struct topic_message message;

};

class Topic_Message_Parser {
public:
	string result;

	void parse_message(struct topic_message message){
		this->message = message;
		get_ip();
		get_port();
		get_topic();
		parse_data();
		make_result();
	}

	void get_ip(){
		struct in_addr aux = {.s_addr = message.ip_udp};
		ip_udp = inet_ntoa(aux);
	}

	void get_port(){
		uint16_t port = ntohs(message.port_udp);
		this->port = to_string(port);
	}

	void get_topic(){
		char aux[100];
		strncpy(aux, (const char *)message.message.topic, 50);
		topic = aux;
	}

	void parse_data(){
		switch(message.message.type){
		case 0:
			parse_int();
			break;
		case 1:
			parse_short();
			break;
		case 2:
			parse_float();
			break;
		case 3:
			parse_string();
			break;
		defalut:
			this->data = "ERROR WHILE PARSING";
		}
	}

	void parse_int(){
		type = "INT";
		uint8_t sign = *(uint8_t *)message.message.data;
		int *data = (int*)(message.message.data + 1);
		int number = ntohl(*data);

		if(sign)
			number *= -1;

		this->data = to_string(number);
	}

	void parse_short(){
		type = "SHORT_REAL";
		uint16_t *data = (uint16_t *)message.message.data;
		double number = ntohs(*data) / 100.0;

		this->data = to_string(number);
	}

	void parse_float(){
		type = "FLOAT";
		uint8_t sign = *(uint8_t *)message.message.data;
		uint32_t data = *(uint32_t *)(message.message.data + 1);
		uint8_t power = *(uint8_t *)(message.message.data + 5);

		float number = ntohl(data) * pow(10, -1 * power);

		if(sign)
			number *= -1;

		this->data = to_string(number);
	}

	void parse_string(){
		type = "STRING";
		this->data = (char *)message.message.data;
	}

	void make_result(){
		result = ip_udp + ":" + port + " - " + topic + " - " + type + " - " + data;
	}

	string get_result(){
		return result;
	}
private:
	struct topic_message message;
	string ip_udp;
	string port;
	string topic;
	string type;
	string data;
};

class Multiplexer{
public:
	Multiplexer (){}
	Multiplexer (int tcpfd){
		this->t = new TCP_Connect(tcpfd);
		this->parser = new Topic_Message_Parser();
		this->tcpfd = tcpfd;
		num_sockets = 0;
		add_fd(STDIN);
		add_fd(tcpfd);
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
				if(poll_fds[i].fd == tcpfd){
					t->recv_message();
					parser->parse_message(t->get_message());
					cout << parser->get_result() << endl;
				}else if(poll_fds[i].fd == STDIN){
					string test;
					cin >> test;
					cout << "am afisat " << test << endl; 
				}
			}
		}
	}

private:
	TCP_Connect *t;
	Topic_Message_Parser *parser;
	vector<struct pollfd> poll_fds;
	int tcpfd;
	int num_sockets;
};


int main(int argc, char *argv[]){
	Client c(argv[1], argv[2], argv[3]);
	c.start();
	Multiplexer x(c.get_fd());
	while(1){
		x.poll_wait();
		x.check_events();
	}

}
