all: build

build: server subscriber

server:
	g++ -std=c++11 -Wall -Wextra server.cpp -o server

subscriber:
	g++ -std=c++11 -Wall -Wextra subscriber.cpp -o subscriber

clean:
	rm -f server subscriber