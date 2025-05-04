#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <cstdint>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define DEBUG 0
#define STDIN 0

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#define ERR(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): %s\n", __FILE__, __LINE__, call_description); \
    }                                                                          \
  } while (0)


int send_all(int fd, void *buf, size_t len, int flags);
int recv_all(int fd, void *buf, size_t len, int flags);
#endif
