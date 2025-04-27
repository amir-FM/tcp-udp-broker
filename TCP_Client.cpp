#include "TCP_Client.h"

TCP_Client::TCP_Client() : listenfd(-1){};

TCP_Client::TCP_Client(int listenfd) : listenfd(listenfd){}

int TCP_Client::recv_message(){
	int rc = recv(listenfd, &message, sizeof(struct topic_message), 0);
	DIE(rc < 0, "recv");
	
	if(rc == 0)
		return -1;
	return 0;

}

void TCP_Client::send_message(struct subscribe_message message){
	int rc = send(listenfd, &message, sizeof(message), 0);
	DIE(rc < 0, "send");
	if(DEBUG)std::cout << "sent message\n";
}

int TCP_Client::get_listenfd(){
	return listenfd;
}

struct topic_message TCP_Client::get_message(){
	return message;
}
