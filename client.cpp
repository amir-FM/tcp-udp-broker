#include <cmath>
#include "helper.h"
#include "protocols.h"

using namespace std;


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
	
	string get_id(){
		return id;
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

		int enable = 1;
		rc = setsockopt(tcpfd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));
		DIE(rc < 0, "setsockopt");

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

	int recv_message(){
		int rc = recv(listenfd, &message, sizeof(struct topic_message), 0);
		DIE(rc < 0, "recv");
		
		if(rc == 0)
			return -1;
		return 0;

	}

	int send_message(struct subscribe_message message){
		int rc = send(listenfd, &message, sizeof(message), 0);
		if(DEBUG)cout << "sent message\n";
		if(rc <= 0)
			return -1;
		return 0;
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

		double number = ntohl(data) * pow(10, -1 * power);

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

class Subscribe_Message_Parser{
public:
	struct subscribe_message message;

	void clean_message(){
		memset(&message, 0, sizeof(message));
	}

	void parse_string(string verb, string id){
		clean_message();
		strncpy((char *)message.clid, id.data(), 11);
		string subject;
		cin >> subject;
		if(DEBUG)cout << "read: " << verb << " " << subject << endl;
		if(verb == "subscribe"){
			subscribe(subject);
			//TODO: de refacut aceasta parte
			cout << "Subscribed to topic " << subject << endl;
		}else if(verb == "unsubscribe"){
			unsubscribe(subject);
			//TODO: de refacut aceasta parte
			cout << "Unsubscribed from topic " << subject << endl;
		}
	}

	void subscribe(string s){
		message.flag = 0;
		strncpy((char *)message.data, s.data(), 50);
	}

	void unsubscribe(string s){
		message.flag = 1;
		strncpy((char *)message.data, s.data(), 50);
	}

	struct subscribe_message get_message(){
		return message;
	}
private:

};

class Multiplexer{
public:
	Multiplexer (){}
	Multiplexer (int tcpfd, string id){
		this->t = new TCP_Connect(tcpfd);
		this->parser = new Topic_Message_Parser();
		this->spar = new Subscribe_Message_Parser();
		this->tcpfd = tcpfd;
		this->id = id;
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

	int check_events(){
		for(int i = 0; i < num_sockets; i++){
			if(poll_fds[i].revents & POLLIN){
				if(poll_fds[i].fd == tcpfd){
					int rc = t->recv_message();
					if(rc == -1)return -1;
					parser->parse_message(t->get_message());
					cout << parser->get_result() << endl;
				}else if(poll_fds[i].fd == STDIN){
					string verb;
					cin >> verb;
					if(verb == "exit")
						return -1;
					spar->parse_string(verb, id);
					int rc = t->send_message(spar->get_message());
					if(rc == -1)return -1;
				}
			}
		}
		return 0;
	}

private:
	TCP_Connect *t;
	Topic_Message_Parser *parser;
	Subscribe_Message_Parser *spar;
	vector<struct pollfd> poll_fds;
	string id;
	int tcpfd;
	int num_sockets;
};


int main(int argc, char *argv[]){
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	Client c(argv[1], argv[2], argv[3]);
	c.start();
	Multiplexer x(c.get_fd(), c.get_id());
	while(1){
		x.poll_wait();
		int rc = x.check_events();
		if(rc == -1)
			break;
	}
	c.stop();
}
