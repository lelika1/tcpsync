#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

typedef struct {
    int network_fd;
    bool is_active;
    FILE *disk_fd;
} connection_t;

typedef struct {
    int idx;
    connection_t *connections;
    int connections_size;
    int active_connections;
} worker_t;

void fill_fd_set(const worker_t *worker, fd_set *fdset) {
    int i = 0;
    FD_ZERO(fdset);
    for (i = 0; i < worker->connections_size; ++i) {
        if (worker->connections[i].is_active) {
            FD_SET(worker->connections[i].network_fd, fdset);
        }
    }
}

void *worker_routine(void *args) {
    int i = 0;
    int ack = 0;
    message_t msg;
    fd_set read_fd_set;
    connection_t *conn = NULL;

    worker_t *worker = (worker_t *)args;
    while (worker->active_connections > 0) {
        fill_fd_set(worker, &read_fd_set);
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) <= 0) {
            continue;
        }

        for (i = 0; i < worker->connections_size; ++i) {
            conn = &worker->connections[i];
            if (!conn->is_active || !FD_ISSET(conn->network_fd, &read_fd_set)) {
                continue;
            }

            read_msg(conn->network_fd, &msg);
            if (msg.is_finish == 1) {
                conn->is_active = false;
                --worker->active_connections;
                close(conn->network_fd);
                fclose(conn->disk_fd);
            } else {
                fwrite(msg.msg, msg.body_size, 1, conn->disk_fd);

                ack = htonl(msg.msg_id);
                send(conn->network_fd, &ack, 4, 0);
            }
        }
    }

    return NULL;
}

int start_server() {
    struct sockaddr_in server_addr = init_server_addr();
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        printf("Socket creation failed.\n");
        return -1;
    }

    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        close(socket_fd);
        printf("Server: bind failed.\n");
        return -1;
    }

    if (listen(socket_fd, N) != 0) {
        close(socket_fd);
        printf("Server: listen failed.\n");
        return -1;
    }
    return socket_fd;
}

int main(int argc, char *argv[]) {
    int i = 0, status = 0;

    pthread_t workers[NUM_WORKERS];
    worker_t workers_args[NUM_WORKERS];

    connection_t connections[N];
    char file_name[20];

    int socket_fd = start_server();
    if (socket_fd == -1) {
        printf("Server launch failed.\n");
        return 1;
    }

    for (i = 0; i < N; ++i) {
        connections[i].network_fd = accept(socket_fd, (struct sockaddr *)NULL, NULL);
        if (sprintf(file_name, "./%d.txt", i) < 0) {
            printf("sprintf() failed");
            return 1;
        }

        connections[i].disk_fd = fopen(file_name, "w");
        if (connections[i].disk_fd == NULL) {
            printf("Failed to open %s.", file_name);
            return 1;
        }
        connections[i].is_active = true;
    }

    connection_t *start_conn = connections;
    int connections_left = N;
    for (i = 0; i < NUM_WORKERS; ++i) {
        workers_args[i].idx = i;
        workers_args[i].connections = start_conn;
        workers_args[i].connections_size =
            connections_left < CONNECTIONS_PER_WORKER ? connections_left : CONNECTIONS_PER_WORKER;
        workers_args[i].active_connections = workers_args[i].connections_size;
        start_conn += workers_args[i].connections_size;
        connections_left -= workers_args[i].connections_size;

        status = pthread_create(&workers[i], NULL, worker_routine, &workers_args[i]);
        if (status != 0) {
            printf("Create thread %d failed, status = %d\n", i, status);
            return -1;
        }
    }

    for (i = 0; i < NUM_WORKERS; ++i) {
        status = pthread_join(workers[i], NULL);
        if (status != 0) {
            printf("Join thread %d failed, status = %d\n", i, status);
        }
    }

    return 0;
}