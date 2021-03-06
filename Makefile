all: server client

server: server.c utils.h utils.c
	gcc -o server server.c utils.c -lpthread -I.

client: client.c utils.h utils.c
	gcc -o client client.c utils.c -lpthread -I.

run_server: server client
	./server

run: server client
	./server &
	time -p ./client

clean:
	rm -f *.txt client server