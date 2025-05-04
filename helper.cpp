#include "helper.h"

int send_all(int fd, void *buf, size_t len, int flags){
	int rc;
	size_t sent = 0;
	while(sent != len){
		rc = send(fd, (void*)((char*)buf + sent), len - sent, flags);
		if(rc <= 0)
			return rc;
		sent += rc;
	}
	return sent;
}

int recv_all(int fd, void *buf, size_t len, int flags){
	int rc;
	size_t recved = 0;
	while(recved != len){
		rc = recv(fd, (void*)((char*)buf + recved), len - recved, flags);
		if(rc <= 0)
			return rc;
		recved += rc;
	}
	return recved;
}
