#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

// Number of messages every client thread retains in memory unacked.

const uint32_t MESSAGES_COUNT = 10000;  // 5000000
const uint32_t MEMUSED_MB = MAX_INFLIGHT_CLIENT * MESSAGE_SIZE * N / 1024 / 1024;

const int MAX_CONNECTION_RETRIES = 5;

int read_acks(int fd, uint32_t *count, uint32_t *ids) {
    int err = read_n_bytes(fd, sizeof(uint32_t), (char *)count);
    if (err != 0) {
        return err;
    }
    *count = ntohl(*count);
    return read_n_bytes(fd, sizeof(uint32_t) * (*count), (char *)ids);
}

void *client_routine(void *args) {
    int socket_fd = 0, retry = 0;
    int thread_idx = *(int *)args;

    int msg_len = 0;
    char *msg_buf = NULL;
    int inflight = 0;
    uint32_t acked_idx = 0;

    struct sockaddr_in server_addr = init_server_addr();
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        printf("[%d] Failed to create a socket.\n", thread_idx);
        return NULL;
    }

    while (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        if (++retry == MAX_CONNECTION_RETRIES) {
            printf("[%d] Failed to connect to server.\n", thread_idx);
            return NULL;
        }
        printf("[%d] Sleep and retry #%d\n", thread_idx, retry);
        sleep(1);
    }

    // Allocate on heap, since we might need a lot of memory.
    msg_buf = malloc(MESSAGE_SIZE * MAX_INFLIGHT_CLIENT);
    int msg_sent = 0;
    uint32_t num_acks = 0;
    uint32_t acks[MAX_INFLIGHT_SERVER];
    while (msg_sent < MESSAGES_COUNT) {
        if (inflight < MAX_INFLIGHT_CLIENT) {
            msg_len = prepare_msg_buf(thread_idx, inflight, msg_buf + MESSAGE_SIZE * inflight);
            send(socket_fd, msg_buf + MESSAGE_SIZE * inflight, msg_len, 0);
            ++inflight;
            ++msg_sent;
            DEBUGF("[%d] msg #%d\n", thread_idx, msg_sent);
            continue;
        }

        read_acks(socket_fd, &num_acks, acks);
        DEBUGF("[%d] read #%d acks\n", thread_idx, acked_idx_count);
        for (int i = 0; i < num_acks; ++i) {
            if (msg_sent >= MESSAGES_COUNT) {
                inflight -= (num_acks - i);
                DEBUGF("[%d] nothing to send. now inflight %d\n", thread_idx, inflight);
                break;
            }

            acked_idx = ntohl(acks[i]);
            // Re-using the place for the acked message to send a new one.
            msg_len = prepare_msg_buf(thread_idx, acked_idx, msg_buf + MESSAGE_SIZE * acked_idx);
            send(socket_fd, msg_buf + MESSAGE_SIZE * acked_idx, msg_len, 0);
            ++msg_sent;
            DEBUGF(" [%d] msg #%d on a vacant place %d\n", thread_idx, msg_sent, acked_idx);
        }
    }

    // We've sent all messages - let's send a closing message.
    DEBUGF("[%d] Closing. inflight %d\n", thread_idx, inflight);
    msg_len = prepare_closing_buf(msg_buf);
    send(socket_fd, msg_buf, msg_len, 0);

    while (inflight > 0) {
        read_acks(socket_fd, &num_acks, acks);
        inflight -= num_acks;
    }

    free(msg_buf);
    return NULL;
}

int main(int argc, char *argv[]) {
    int status = 0;
    pthread_t threads[N];
    int thread_idxs[N];

    for (int i = 0; i < N; ++i) {
        thread_idxs[i] = i;
        status = pthread_create(&threads[i], NULL, client_routine, &thread_idxs[i]);
        if (status != 0) {
            printf("Create thread %d failed, status = %d\n", i, status);
            return status;
        }
    }

    for (int i = 0; i < N; ++i) {
        status = pthread_join(threads[i], NULL);
        if (status != 0) {
            printf("Join thread %d failed, status = %d\n", i, status);
            return status;
        }
    }
    return 0;
}