#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

typedef struct {
    bool is_active;
    int network_fd;
    FILE *disk_fd;

    // IDs of unacked messages we haven't fflushed onto disk yet.
    uint32_t unacked_ids[MAX_INFLIGHT_SERVER];
    // Number of such messages.
    int unacked;
} connection_t;

typedef struct {
    // Worker index.
    int idx;
    // Connections in [connections; connections + connections_size[ belong to us.
    connection_t *connections;
    int connections_size;
    // Number of connections that are still sending data.
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

void flush_and_ack(connection_t *conn) {
    if (conn->unacked == 0) {
        return;
    }

    fflush(conn->disk_fd);

    uint32_t acks[MAX_INFLIGHT_SERVER + 1];
    acks[0] = htonl(conn->unacked);
    for (int i = 0; i < conn->unacked; ++i) {
        acks[i + 1] = conn->unacked_ids[i];
    }

    send(conn->network_fd, &acks, sizeof(uint32_t) * (conn->unacked + 1), 0);
    conn->unacked = 0;
}

void *worker_routine(void *args) {
    message_t msg;
    fd_set read_fd_set;

    worker_t *worker = (worker_t *)args;
    while (worker->active_connections > 0) {
        fill_fd_set(worker, &read_fd_set);
        int err = select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);
        if (err <= 0) {
            printf("select() failed with %d", err);
            continue;
        }

        for (int i = 0; i < worker->connections_size; ++i) {
            connection_t *conn = &worker->connections[i];
            if (!conn->is_active || !FD_ISSET(conn->network_fd, &read_fd_set)) {
                continue;
            }

            recv_msg(conn->network_fd, &msg);
            if (msg.msg_id == CLOSING_ID) {
                conn->is_active = false;
                --worker->active_connections;
                flush_and_ack(conn);
            } else {
                fwrite(msg.payload, msg.body_size, 1, conn->disk_fd);
                conn->unacked_ids[conn->unacked] = htonl(msg.msg_id);
                ++conn->unacked;
                if (conn->unacked == MAX_INFLIGHT_SERVER) {
                    flush_and_ack(conn);
                }
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
    int socket_fd = start_server();
    if (socket_fd == -1) {
        printf("Server launch failed.\n");
        return 1;
    }

    char file_name[50];
    connection_t *connections = malloc(sizeof(connection_t) * N);
    for (int i = 0; i < N; ++i) {
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
        connections[i].unacked = 0;
    }

    pthread_t workers[NUM_WORKERS];
    worker_t workers_args[NUM_WORKERS];

    connection_t *start_conn = connections;
    int connections_left = N;

    static const int CONNECTIONS_PER_WORKER =
        (N % NUM_WORKERS == 0) ? N / NUM_WORKERS : (N / NUM_WORKERS + 1);

    for (int i = 0; i < NUM_WORKERS; ++i) {
        worker_t *worker = &workers_args[i];
        worker->idx = i;
        worker->connections = start_conn;
        worker->connections_size =
            connections_left < CONNECTIONS_PER_WORKER ? connections_left : CONNECTIONS_PER_WORKER;
        worker->active_connections = worker->connections_size;
        start_conn += worker->connections_size;
        connections_left -= worker->connections_size;

        int status = pthread_create(&workers[i], NULL, worker_routine, worker);
        if (status != 0) {
            printf("Create thread %d failed, status = %d\n", i, status);
            return -1;
        }
    }

    for (int i = 0; i < NUM_WORKERS; ++i) {
        int status = pthread_join(workers[i], NULL);
        if (status != 0) {
            printf("Join thread %d failed, status = %d\n", i, status);
        }
    }

    for (int i = 0; i < N; ++i) {
        close(connections[i].network_fd);
        fclose(connections[i].disk_fd);
    }
    free(connections);
    return 0;
}

// Constant below compute the approx memory footprint of the server (only for debugging).
const int MEMUSED_MB = (NUM_WORKERS * sizeof(worker_t) + sizeof(connection_t) * N) / 1024 / 1024;