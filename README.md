# tcp-udp-broker

A small pub/sub message broker over raw sockets: UDP clients publish data on arbitrary topics, and TCP clients subscribe (with wildcard support) to get notified whenever something matching comes in.

```
UDP publisher --topic/type/payload--> server --topic data--> TCP subscriber
```

The server is single-threaded and multiplexes every TCP connection plus the UDP socket with `poll()` — no threads, no polling loops, no blocking on a slow client.

## Layout

- `tcp_server/` — the broker: TCP + UDP listener, subscription tracking, topic matching, fan-out
- `udp_client/` — a UDP publisher used to generate test traffic

## Application-layer protocol

TCP has no notion of message boundaries, so everything sent over the TCP connection is framed with fixed-size structs rather than relying on `recv()` returning one "message" at a time.

**Server → TCP client** (a UDP publish, forwarded to interested subscribers):

```cpp
struct topic_message {
    uint32_t ip_udp;
    uint16_t port_udp;
    struct UDP_Message message;
};
```

**TCP client → server** (login / subscribe / unsubscribe):

```cpp
struct subscribe_message {
    uint8_t flag;      // 0 = subscribe, 1 = unsubscribe, 2 = login
    uint8_t clid[11];
    uint8_t data[50];  // topic, for subscribe/unsubscribe
};
```

Subscriptions and connection state are tracked with:

```cpp
map<string, int> users;              // client_id -> socket fd
map<string, set<string>> topics;     // client_id -> subscribed topics
```

For every UDP packet received, the server looks up which client IDs are both connected and subscribed to that topic, and forwards the message only to them.

## Features

- I/O multiplexing with `poll()` on the server — TCP listener, UDP socket, all client sockets, and stdin in one loop
- Custom length-prefixed framing over TCP to handle message coalescing/fragmentation correctly
- `TCP_NODELAY` enabled on TCP sockets for low-latency delivery
- Wildcard topic subscriptions:
  - `*` matches zero or more topic levels
  - `+` matches exactly one topic level
- A client subscribed to overlapping topics (e.g. an exact match and a wildcard covering it) only gets each message once
- Subscriptions persist across a client's disconnect/reconnect, and are only dropped on explicit `unsubscribe`
- Duplicate client IDs are rejected while the original is still connected
- Payload decoding for `INT`, `SHORT_REAL`, `FLOAT`, and `STRING`, printed in human-readable form

## Building

```bash
make
```

## Running

```bash
./server <PORT>
```

```bash
./subscriber <CLIENT_ID> <SERVER_IP> <SERVER_PORT>
```

From a subscriber's stdin:

```
subscribe home/kitchen/temperature
subscribe home/+/temperature
unsubscribe home/kitchen/temperature
exit
```

Incoming messages print as:

```
<UDP_CLIENT_IP>:<UDP_CLIENT_PORT> - <TOPIC> - <TYPE> - <VALUE>
```

`exit` on the server shuts it down along with every connected TCP client.
