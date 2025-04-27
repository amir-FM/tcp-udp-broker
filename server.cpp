#include <sstream>
#include <climits>
#include "helper.h"
#include "protocols.h"


using namespace std;

class Regex {
public:
	int check(string regex, string str){
		if(regex == "*")
			return 1;

		stringstream reg_stream(regex);
		stringstream str_stream(str);
		reg_stream << regex;
		str_stream << str;

		int rc = algo(reg_stream, str_stream);
		return rc;
	}

private:
	int algo(stringstream &regex, stringstream &str){
		int is_valid = 0;
		string tokr, toks;

		while(getline(regex, tokr, '/') && getline(str, toks, '/')){
			if(DEBUG)cout << tokr << " " << toks << endl;
			if(tokr == "*"){
				if(!getline(regex, tokr, '/'))return 1;
				while(getline(str, toks, '/')){
					if(tokr == toks)
						break;
				}
				if(tokr != toks)
					return 0;
			}else if(tokr != "+"){
				if(tokr != toks)
					return 0;
			}
		}

		if(getline(regex, tokr, '/') || getline(str, toks, '/'))
			return 0;

		return 1;
	}
};

class Share {
public:
	Share () {
		this->rgx = new Regex();
	}

	int connect_user(string id, int fd){
		if(user_in_use(id)){
			cout << "Client " << id << " already connected.\n";
			return -1;
		}

		users[id] = fd;
		if(DEBUG)cout << "inserted: " << fd << endl;
		return 0;
	}

	int get_fd(string id){
		try {
			return users.at(id);
		}catch(...) {
			return -1;
		}
	}

	string disconnect_user(int fd){
		if(DEBUG)cout << "caut: " << fd << endl;
		if(DEBUG)cout << "dim: " << users.size() << endl;
		for(auto it : users){
			if(DEBUG)cout << "pereche: " << it.first << ":" << it.second << endl;
			if(it.second == fd){
				disconnect_user(it.first);
				return it.first;
			}
		}
		return "NOT SUPPOSED TO ARRIVE HERE";
	}

	map<string, set<string>> get_topics(){
		return topics;
	}

	int check_user_topic(string id, string topic){
		set<string> s = topics[id];

		for(auto it : s)
			if(check_topic(it, topic))
				return 1;
		return 0;
	}

	void print_all_users(){
		for(auto it : users)
			if(DEBUG)cout << "(" << it.first << ":" << it.second << ") ";
		if(DEBUG)cout << endl;
	}

	void print_active_users(){
		for(auto it : users)
			if(it.second != -1)
				if(DEBUG)cout << "(" << it.first << ":" << it.second << ") ";
		if(DEBUG)cout << endl;
	}

	int add_topic_to_user(string topic, string id){
		if(!user_exists(id))
			return -1;

		topics[id].insert(topic);
		return 0;
	}

	int remove_topic_from_user(string topic, string id){
		if(!user_exists(id))
			return -1;

		topics[id].erase(topic);

		return 0;
	}

	void print_all_topics(){
		for(auto it : topics){
			if(DEBUG)cout << it.first << ": ";
			for(auto topic : it.second)
				if(DEBUG)cout << topic << " ";
			if(DEBUG)cout << endl;
		}
	}

private:
	map<string, int> users;
	map<string, set<string>> topics;
	Regex *rgx;

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

	int check_topic(string a, string b){
		int rc = rgx->check(a, b);
		if(DEBUG)cout << "checked: " << a << " , " << b << " with rc: " << rc << endl;
		return rc;
	}
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
		if(DEBUG)cout << flag << " " << id << " " << topic << endl;
	}

	int login(){
		if(flag != 2){
			if(DEBUG)cout << "Message not correct\n";
			ERR(1, "login message not correct");
			return -1;
		}

		int rc = s->connect_user(id, fd);
		s->print_all_users();

		if(rc < 0)
			return rc;
		return 0;
	}

	string get_clid(){
		return id;
	}

	void logout(int fd){
		if(DEBUG)cout << "sunt in logout\n";
		id = s->disconnect_user(fd);
		s->print_all_users();
	}

	map<string, set<string>> get_topics(){
		return s->get_topics();
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

	int check_user_topic(string id, string topic){
		return s->check_user_topic(id, topic);
	}

	int get_fd(string id){
		return s->get_fd(id);
	}

private:
	Share *s;
	struct subscribe_message message;
	int fd;
	string id;
	string topic;
	uint8_t flag;

	void subscribe(){
		s->add_topic_to_user(topic, id);
	}

	void unsubscribe(){
		s->remove_topic_from_user(topic, id);
	}
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

		rc = listen(tcpfd, INT_MAX);
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

	int recv(){
		clean_message();

		socklen_t clen = sizeof(cl_addr);

		int rc = recvfrom(listenfd, &message, sizeof(struct UDP_Message), 0, (struct sockaddr *)&cl_addr, &clen);
		ERR(rc <= 0, "recv from udp incorrectly");
		
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

	string get_topic(){
		string aux = (char *)message.topic;
		return aux;
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

	TCP_Connect(int listenfd, Subscribe_Message_Parser *p){
		this->listenfd = listenfd;
		this->p = p;
	}

	int new_connection(){
		int rc;
		struct sockaddr_in cli_addr;
		socklen_t cli_len = sizeof(cli_addr);

		int newfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
		DIE(newfd < 0, "accept");

		int enable = 1;
		rc = setsockopt(newfd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));

		//login
		rc = recv_smess(newfd);
		if(rc < 0)return -1;

		p->set_message(get_smess(), newfd);
		rc = p->login();
		if(rc < 0){
			close_connection(newfd, 0);
			return -1;
		}

		cout << "New client " << p->get_clid() << " connected from " << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << ".\n";

		return newfd;
	}

	void close_connection(int fd, int print){
		close(fd);
		if(print)cout << "Client " << p->get_clid() << " disconnected.\n";
	}

	int send_topic_message(int fd, struct topic_message message){
		int rc;

		rc = send(fd, &message, sizeof(message), 0);

		if(DEBUG)cout << "Sent message to " << fd << endl;
		ERR(rc <= 0, "failed to send topic message");
		if(rc <= 0)
			return -1;
		return 0;
	}

	int get_listenfd(){
		return listenfd;
	}

	int recv_smess(int fd){
		int rc = recv(fd, &smess, sizeof(smess), 0);
		
		ERR(rc <= 0, "failed to recv subscriber message");
		if(rc <= 0)
			return -1;
		return 0;
	}

	struct subscribe_message get_smess(){
		return smess;
	}
	
private:
	Subscribe_Message_Parser *p;
	int listenfd;
	struct subscribe_message smess;
};

class Multiplexer{
public:
	Multiplexer(int tcpfd, int udpfd){
		this->p = new Subscribe_Message_Parser();
		this->t = new TCP_Connect(tcpfd, p);
		this->u = new UDP_Connect(udpfd);
		this->tcpfd = tcpfd;
		this->udpfd = udpfd;

		num_sockets = 0;
		add_fd(STDIN);
		add_fd(this->tcpfd);
		add_fd(this->udpfd);
	}

	void poll_wait(){
		int rc = poll(poll_fds.data(), num_sockets, -1);
		DIE(rc < 0, "poll");
	}

	int check_events(){
		int rc;
		for(int i = 0; i < num_sockets; i++){
			if(poll_fds[i].revents & POLLIN){
				if(poll_fds[i].fd == tcpfd){
					int newfd = t->new_connection();
					if(newfd >= 0)
						add_fd(newfd);
				}else if(poll_fds[i].fd == udpfd){
					rc = u->recv();
					if(rc > 0)send_subs();
				}else if(poll_fds[i].fd == STDIN){
					string s;
					cin >> s;
					if(s == "exit")
						return -1;
					ERR(1, "invalid command");
				}else{
					rc = t->recv_smess(poll_fds[i].fd);
					if(rc == -1){
						logout_user(poll_fds[i].fd);
						continue;
					}
					p->set_message(t->get_smess(), poll_fds[i].fd);
					p->parse_message();
				}
			}
		}
		return 0;
	}

private:
	TCP_Connect *t;
	UDP_Connect *u;
	Subscribe_Message_Parser *p;
	vector<struct pollfd> poll_fds;
	int tcpfd, udpfd;
	int num_sockets;

	void add_fd(int fd){
		struct pollfd p = {.fd = fd, .events = POLLIN};
		poll_fds.push_back(p);
		num_sockets++;
		if(DEBUG)cout << "added: " << fd << endl;
	}

	void remove_fd(int fd){
		int i = 0;
		while(i < num_sockets){
			if(poll_fds[i].fd == fd)
				break;
			i++;
		}

		if(i == num_sockets)
			return;

		poll_fds.erase(poll_fds.begin() + i);
		if(DEBUG)cout << "Removed: " << fd << endl;
		num_sockets--;
	}

	void logout_user(int fd){
		p->logout(fd);
		remove_fd(fd);
		t->close_connection(fd, 1);
	}

	void send_subs(){
		struct topic_message payload = wrap_message(u->get_message());
		string topic = u->get_topic();

		map<string, set<string>> topics = p->get_topics();

		for(auto it : topics){
			int fd = p->get_fd(it.first);
			if(fd < 0)continue;
			if(p->check_user_topic(it.first, topic)){
				int rc = t->send_topic_message(fd, payload);
				if(rc == -1)
					logout_user(fd);
			}
		}
	}

	void send_all(struct UDP_Message message){
		struct topic_message payload = wrap_message(message);

		for(auto it : poll_fds)
			if(it.fd != tcpfd && it.fd != udpfd && it.fd != STDIN){
				int rc = t->send_topic_message(it.fd, payload);
				if(rc == -1)logout_user(it.fd);
			}
	}

	struct topic_message wrap_message(struct UDP_Message message){
		struct topic_message payload = {.ip_udp = u->get_client_ip(), .port_udp = u->get_client_port(), .message = message};

		return payload;
	}
};

int main(int argc, char *argv[]){
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	Server s(argv[1]);
	s.start();

	Multiplexer x(s.get_tcp_fd(), s.get_udp_fd());

	while(1){
		x.poll_wait();
		int rc = x.check_events();
		if(rc == -1)
			break;
	}

	s.stop();
}

