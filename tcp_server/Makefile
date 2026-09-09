all: server subscriber

server: server.cpp
	g++ -g server.cpp helper.cpp -o server

subscriber: subscriber.cpp
	g++ -g subscriber.cpp helper.cpp -o subscriber

clean:
	-rm server subscriber
