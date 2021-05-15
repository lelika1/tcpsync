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

int readbytes(int conn_fd, char* to, size_t n) {
    int got = 0;
    while (n > 0) {
        got = recv(conn_fd, to, n, 0);
        if (got == -1) {
            return -1;
        }
        to += got;
        n -= got;
    }
    return 0;
}

// Returns size of data in `out`.
int deserialize_msg(int conn_fd, message_t *out) {
    char buf[9];
    if (readbytes(conn_fd, buf, 9) == -1) {
        printf("Failed to read header.\n");
        return -1;
    }
    out->msg_id = ntohl(*((uint32_t *)buf));
    out->body_size = ntohl(*((uint32_t *)(buf+4)));
    if (out->body_size > sizeof(out->msg)) {
        printf("Body size (%d) is bigger than max message size (%ld)\n", out->body_size, sizeof(out->msg)); 
        return -1;
    }
    out->is_finish = buf[8];
    if (readbytes(conn_fd, out->msg, out->body_size) == -1) {
        printf("Failed to read body with size %d.\n", out->body_size);
        return -1;
    }
    return 0;
}

void *worker_routine(void *args) {
    int idx;
    init_args_t *descr = (init_args_t *) args;
    for (idx = 0; idx < descr->conn_count; ++idx) {
        printf("%d -> %d\n", descr->idx, descr->connections[idx].conn_fd);
    }
    
    message_t msg;
    for (idx = 0; idx < descr->conn_count; ++idx) {
        deserialize_msg(descr->connections[idx].conn_fd, &msg);
        fwrite(msg.msg, msg.body_size, 1, descr->connections[idx].out_fd);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int i = 0;
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

    for (i = 0; i < N; ++i) {
        connections[i].conn_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL);
        if (sprintf(file_name, "./%d.txt", i) < 0) {
            printf("sprintf() failed");
            return 1;
        }

        // printf("File: %s", file_name);
        connections[i].out_fd = fopen(file_name, "w");
        if (connections[i].out_fd == NULL) {
            printf("Failed to open %s.", file_name);
            return 1;
        }
    }

    conn_descr_t *start_conn = connections;
    int connections_left = N;
    for (i = 0; i < WORKERS; ++i) {        
        threads_args[i].idx = i;
        threads_args[i].connections = start_conn;
        threads_args[i].conn_count = connections_left < CONNECTIONS_PER_WORKER ? connections_left : CONNECTIONS_PER_WORKER;
        start_conn += threads_args[i].conn_count;
        connections_left -= threads_args[i].conn_count;

        status = pthread_create(&threads[i], NULL, worker_routine, &threads_args[i]);
        if (status != 0) {
            // TODO proccess error
            printf("Create thread %d failed, status = %d\n", i, status);
        }
    }


    for (i = 0; i < WORKERS; ++i) {
        status = pthread_join(threads[i], NULL);
        if (status != 0) {
            // TODO proccess error
            printf("Join thread %d failed, status = %d\n", i, status);
        }
    }
    
    for (i = 0; i < N; ++i) {
        close(connections[i].conn_fd);
        fclose(connections[i].out_fd);
    }
    return 0;
}