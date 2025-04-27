#ifndef _PROTOCOLS_H
#define _PROTOCOLS_H 1

struct UDP_Message{
	uint8_t topic[50];
	uint8_t type;
	uint8_t data[1500];
}; 

struct topic_message {
	uint32_t ip_udp;
	uint16_t port_udp;
	struct UDP_Message message;
};

struct subscribe_message {
	uint8_t flag; // 0 - subscribe, 1 - unsubscribe, 2 - login;
	uint8_t clid[11];
	uint8_t data[50];
};

#endif
