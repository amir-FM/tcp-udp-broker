#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <iostream>
#include <cstdint>
#include <sys/socket.h>
#include "protocols.h"
#include "helper.h"

class TCP_Client{
public:
	TCP_Client();
	TCP_Client(int listenfd);

	int recv_message();
	void send_message(struct subscribe_message message);
	int get_listenfd();
	struct topic_message get_message();
	
private:
	int listenfd;
	struct topic_message message;

};

#endif // TCP_CLIENT_H
