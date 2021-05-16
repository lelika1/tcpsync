.PHONY: cleanup

all: server client

server: server.c utiles.h utiles.c
	gcc -o server server.c utiles.c -lpthread -I.

client: client.c utiles.h utiles.c
	gcc -o client client.c utiles.c -lpthread -I.

run_server: server client
	./server

cleanup:
	rm -f *.txt client server