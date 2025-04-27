# Tema 2 PCOM - TCP&UDP Client Server Application

## Protocol Nivel Aplicatie

### Server -> Client TCP

Pachetul primit de la clientii UDP este *encapsulat* si transmis mai departe
catre clientii TCP care sunt abonati la topicul respectiv.

```
struct topic_message {
	uint32_t ip_udp;
	uint16_t port_udp;
	struct UDP_Message message;
};
```

- ip_udp = Adresa IP a clientului UDP care a trimis mesajul
- port_udp = Portul clientului UDP care a trimis mesajul
- message = mesajul trimis de clientul UDP

### Client TCP -> Server

```
struct subscribe_message {
	uint8_t flag;
	uint8_t clid[11];
	uint8_t data[50];
};
```

- flag = tipul mesajului:
    - 0 = abonare de la topic
    - 1 = dezabonare de la topic
    - 2 = transmiterea de *<CLIENT-ID>* la inceputul conexiunii - login
- clid = *<CLIENT-ID>* specific clientului
- data = pentru primele doua optiuni ale *flag*, reprezinta topicul

## Gestiunea subscriptiilor

```
	map<string, int> users;
	map<string, set<string>> topics;
```

- users = hashmap cu perechea (user_id, fd)
- topics = hashmap cu perechea (user_id, set de topicuri)

Pentru fiecare mesaj primit de la un client UDP, aceste structuri sunt
interogate pentru a trimit mesaje TCP doar acelor utilizatori care, in primul
rand sunt conectati (aici se foloseste prima structura) si care sunt abonati la
topicul transmis.
