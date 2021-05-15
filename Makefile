.PHONY: cleanup

all: server client

server: server.c config.h
	gcc -o server server.c -lpthread -I.

client: client.c config.h
	gcc -o client client.c -lpthread -I.

run_server: server client
	./server

cleanup:
	rm -f *.txt client server