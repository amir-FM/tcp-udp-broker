#include <iostream>
#include <vector>
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

#define DEBUG 1
#define MAX_CONNECTIONS 1000

using namespace std;

class Share {
public:
	int connect_user(string id, int fd){
		if(user_in_use(id)){
			cout << "User: " << id << ":" << users[id] << " is in use\n";
			return -1;
		}

		//users.insert({id, fd});
		users[id] = fd;
		cout << "inserted: " << fd << endl;
		return 0;
	}

	int get_fd(string id){
		try {
			return users.at(id);
		}catch(...) {
			return -1;
		}
	}

	int user_in_use(string id){
		if(users.find(id) == users.end())
			return 0;

		return users[id] >= 0;
	}
	
	int user_exists(string id){
		return users.find(id) != users.end();
	}

	int topic_exists(string topic){
		return topics.find(topic) != topics.end();
	}

	void disconnect_user(string id){
		users[id] = -1;
	}

	string disconnect_user(int fd){
		cout << "caut: " << fd << endl;
		cout << "dim: " << users.size() << endl;
		for(auto it : users){
			cout << "pereche: " << it.first << ":" << it.second << endl;
			if(it.second == fd){
				disconnect_user(it.first);
				return it.first;
			}
		}
		return "NOT SUPPOSED TO ARRIVE HERE";
	}

	void print_all_users(){
		for(auto it : users)
			cout << "(" << it.first << ":" << it.second << ") ";
		cout << endl;
	}

	void print_active_users(){
		for(auto it : users)
			if(it.second != -1)
				cout << "(" << it.first << ":" << it.second << ") ";
		cout << endl;
	}

	int add_user_to_topic(string topic, string id){
		if(!user_exists(id))
			return -1;

		topics[topic].insert(id);
		return 0;
	}

	int remove_user_from_topic(string topic, string id){
		if(!user_exists(id))
			return -1;

		if(!topic_exists(topic))
			return -1;

		topics[topic].erase(id);

		return 0;
	}

	void print_all_topics(){
		for(auto it : topics){
			cout << it.first << ": ";
			for(auto user : it.second)
				cout << user << " ";
			cout << endl;
		}
	}

private:
	map<string, int> users;
	map<string, set<string>> topics;
};

class Subscribe_Message_Parser {
public:
	Subscribe_Message_Parser(){
		this->s = new Share();
	}

	void set_message(struct subscribe_message message, int fd){
		this->message = message;
		this->fd = fd;
		this->flag = message.flag;
		this->id = (char *)message.clid;
		this->topic = (char *)message.data;
		cout << flag << " " << id << " " << topic << endl;
	}

	void parse_message(){
		switch(flag){
		case 0:
			subscribe();
			break;
		case 1:
			unsubscribe();
			break;
		default:
			break;
		}
		s->print_all_topics();
	}

	void subscribe(){
		s->add_user_to_topic(topic, id);
	}

	void unsubscribe(){
		s->remove_user_from_topic(topic, id);
	}

	int login(){
		if(flag != 2){
			cout << "Message not correct\n";
			return -1;
		}

		int rc = s->connect_user(id, fd);
		s->print_all_users();

		if(rc < 0)
			return rc;
		return 0;
	}

	void logout(int fd){
		cout << "sunt in logout\n";
		id = s->disconnect_user(fd);
		s->print_all_users();
	}

	Share *get_share(){
		return s;
	}

	string get_clid(){
		return id;
	}
private:
	Share *s;
	struct subscribe_message message;
	int fd;
	string id;
	string topic;
	uint8_t flag;
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
	UDP_Connect(){}

	UDP_Connect(int listenfd){
		this->listenfd = listenfd;
	}

	//int recv_and_print(){
	//	clean_message();

	//	struct sockaddr_in cl_addr;
	//	socklen_t clen = sizeof(cl_addr);

	//	int rc = recvfrom(listenfd, &message, sizeof(struct udp_message), 0, (struct sockaddr *)&cl_addr, &clen);
	//	
	//	if(rc > 0)
	//		cout << message.topic << " " << message.type << "\n" << message.data<< "\n------------------------------------\n\n";

	//	return rc;
	//}

	int recv(){
		clean_message();

		socklen_t clen = sizeof(cl_addr);

		int rc = recvfrom(listenfd, &message, sizeof(struct UDP_Message), 0, (struct sockaddr *)&cl_addr, &clen);
		if(DEBUG)cout << "Recv UDP: " << rc << "bytes\n";
		
		return rc;
	}

	struct UDP_Message get_message(){
		return message;
	}

	uint32_t get_client_ip(){
		return cl_addr.sin_addr.s_addr;
	}

	uint16_t get_client_port(){
		return cl_addr.sin_port;
	}

	int get_listenfd(){
		return listenfd;
	}

private:
	int listenfd;
	struct UDP_Message message;
	struct sockaddr_in cl_addr;

	void clean_message(){
		memset(&message, 0, sizeof(struct UDP_Message));
	}
};

class TCP_Connect{
public:
	TCP_Connect(){};

	TCP_Connect(int listenfd, Subscribe_Message_Parser &p){
		this->listenfd = listenfd;
		this->p = p;
	}

	int new_connection(){
		struct sockaddr_in cli_addr;
		socklen_t cli_len = sizeof(cli_addr);

		int newfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
		DIE(newfd < 0, "accept");

		//login
		int rc = recv_smess(newfd);
		if(rc < 0)return -1;

		p.set_message(get_smess(), newfd);
		rc = p.login();
		if(rc < 0){
			close_connection(newfd);
			return -1;
		}

		cout << "New client " << p.get_clid() << " connected from " << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << "\n";

		return newfd;
	}

	void close_connection(int fd){
		close(fd);
		if(DEBUG)cout << "Client " << p.get_clid() << " disconnected: " << fd << endl;
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

	void send_topic_message(int fd, struct topic_message message){
		int rc;

		rc = send(fd, &message, sizeof(message), 0);
		DIE(rc < 0, "send");

		if(DEBUG)cout << "Sent message to " << fd << endl;
	}

	int get_listenfd(){
		return listenfd;
	}

	int recv_smess(int fd){
		int rc = recv(fd, &smess, sizeof(smess), 0);
		DIE(rc < 0, "recv");
		
		if(rc == 0)
			return -1;
		return 0;
	}

	struct subscribe_message get_smess(){
		return smess;
	}
	
private:
	Subscribe_Message_Parser p;
	int listenfd;
	struct subscribe_message smess;
};

class Multiplexer{
public:
	Multiplexer(TCP_Connect t){
		this->t = t;
		this->tcpfd = t.get_listenfd();
		add_fd(this->tcpfd);

		num_sockets = 1;
	}

	void add_udp(UDP_Connect u){
		this->u = u;
		this->udpfd = u.get_listenfd();
		add_fd(this->udpfd);

		num_sockets++;
	}

	void add_subs(Subscribe_Message_Parser &p){
		this->p = p;
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
					int newfd = t.new_connection();
					if(newfd >= 0)
						add_fd(newfd);
				}else if(poll_fds[i].fd == udpfd){
					u.recv();
					send_all(u.get_message());
				}else{
					int rc = t.recv_smess(poll_fds[i].fd);
					if(rc == -1){
						p.logout(poll_fds[i].fd);
						remove_fd(i);
						t.close_connection(poll_fds[i].fd);
						continue;
					}
					p.set_message(t.get_smess(), poll_fds[i].fd);
					p.parse_message();
				}
				break;
			}
		}
	}

	void send_all(struct UDP_Message message){
		struct topic_message payload = wrap_message(message);
		cout << "wrapped message: " << payload.ip_udp << " " << payload.port_udp << " " << payload.message.topic << " " << payload.message.type << " " << payload.message.data << endl;

		for(auto it : poll_fds)
			if(it.fd != tcpfd && it.fd != udpfd)
				t.send_topic_message(it.fd, payload);
	}

	struct topic_message wrap_message(struct UDP_Message message){
		struct topic_message payload = {.ip_udp = u.get_client_ip(), .port_udp = u.get_client_port(), .message = message};

		return payload;
	}

private:
	TCP_Connect t;
	UDP_Connect u;
	Subscribe_Message_Parser p;
	vector<struct pollfd> poll_fds;
	int tcpfd, udpfd;
	int num_sockets;
};

int main(int argc, char *argv[]){
	Server s(argv[1]);
	s.start();

	Subscribe_Message_Parser p;
	UDP_Connect u(s.get_udp_fd());
	TCP_Connect t(s.get_tcp_fd(), p);
	Multiplexer x(t);
	x.add_udp(u);
	x.add_subs(p);

	while(1){
		x.poll_wait();
		x.check_events();
	}
}

