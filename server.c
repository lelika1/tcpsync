#include "server.h"
#include "config.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void *worker_routine(void *args) {
    int idx;
    init_args_t *descr = (init_args_t *) args;
    for (idx = 0; idx < descr->conn_count; ++idx) {
        printf("%d -> %d\n", descr->idx, descr->connections[idx].conn_fd);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int idx = 0;
    int status = 0;

    pthread_t threads[WORKERS];
    init_args_t threads_args[WORKERS];
    conn_descr_t connections[N];
    char file_name[20];
 
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        // TODO proccess error
        printf("Socket creation failed.\n");
        return 1;
    }
   
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        // TODO proccess error
        printf("Server: bind failed.\n");
        return 1;
    }

    if (listen(socket_fd, N) != 0) {
        // TODO proccess error
        printf("Server: listen failed.\n");
        return 1;
    }

    for (idx = 0; idx < N; ++idx) {
        connections[idx].conn_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL);
        if (sprintf(file_name, "./%d.txt", idx) < 0) {
            printf("sprintf() failed");
            return 1;
        }

        // printf("File: %s", file_name);
        connections[idx].out_fd = fopen(file_name, "w");
        if (connections[idx].out_fd == NULL) {
            printf("Failed to open %s.", file_name);
            return 1;
        }
    }

    conn_descr_t *start_conn = connections;
    int connections_left = N;
    for (idx = 0; idx < WORKERS; ++idx) {        
        threads_args[idx].idx = idx;
        threads_args[idx].connections = start_conn;
        threads_args[idx].conn_count = connections_left < CONNECTIONS_PER_WORKER ? connections_left : CONNECTIONS_PER_WORKER;
        start_conn += threads_args[idx].conn_count;
        connections_left -= threads_args[idx].conn_count;

        status = pthread_create(&threads[idx], NULL, worker_routine, &threads_args[idx]);
        if (status != 0) {
            // TODO proccess error
            printf("Create thread %d failed, status = %d\n", idx, status);
        }
    }

    // close(conn_fd);

    for (idx = 0; idx < WORKERS; ++idx) {
        status = pthread_join(threads[idx], NULL);
        if (status != 0) {
            // TODO proccess error
            printf("Join thread %d failed, status = %d\n", idx, status);
        }
    }
    return 0;
}