all: server subscriber

server: server.cpp
	g++ -g server.cpp -o server

subscriber: client.cpp
	g++ -g client.cpp -o subscriber

clean:
	-rm server subscriber
