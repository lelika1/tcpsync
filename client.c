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

#define MAX_INFLIGHT 1000  // TODO: tune the number

const int MAX_RETRIES = 5;
const uint32_t MESSAGES_COUNT = 10000;  // 5000000

void *client_routine(void *args) {
    int i = 0, socket_fd = 0, retry = 0;
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
        if (++retry == MAX_RETRIES) {
            printf("[%d] Failed to connect to server.\n", thread_idx);
            return NULL;
        }
        printf("[%d] Sleep and retry #%d\n", thread_idx, retry);
        sleep(1);
    }

    // Allocate on heap, since we might need a lot of memory.
    msg_buf = malloc(BUFFER_SIZE * MAX_INFLIGHT);
    for (i = 0; i < MESSAGES_COUNT; ++i) {
        if (inflight < MAX_INFLIGHT) {
            msg_len = prepare_msg_buf(thread_idx, inflight, msg_buf + BUFFER_SIZE * inflight);
            send(socket_fd, msg_buf + BUFFER_SIZE * inflight, msg_len, 0);
            ++inflight;
            continue;
        }

        read_n_bytes(socket_fd, 4, (char *)&acked_idx);
        acked_idx = ntohl(acked_idx);
        // Re-using the place for the acked message to send a new one.
        msg_len = prepare_msg_buf(thread_idx, acked_idx, msg_buf + BUFFER_SIZE * acked_idx);
        send(socket_fd, msg_buf + BUFFER_SIZE * acked_idx, msg_len, 0);
    }

    printf("Thread %d sent all messages - waiting for acks.\n", thread_idx);

    while (inflight > 0) {
        read_n_bytes(socket_fd, 4, (char *)&acked_idx);
        --inflight;
    }

    // We've sent all messages - let's send a closing message.
    msg_len = prepare_closing_buf(msg_buf);
    send(socket_fd, msg_buf, msg_len, 0);

    free(msg_buf);
    return NULL;
}

int main(int argc, char *argv[]) {
    int idx = 0;
    int status = 0;
    pthread_t threads[N];
    int thread_idxs[N];

    for (idx = 0; idx < N; ++idx) {
        thread_idxs[idx] = idx;
        status = pthread_create(&threads[idx], NULL, client_routine, &thread_idxs[idx]);
        if (status != 0) {
            printf("Create thread %d failed, status = %d\n", idx, status);
            return status;
        }
    }

    for (idx = 0; idx < N; ++idx) {
        status = pthread_join(threads[idx], NULL);
        if (status != 0) {
            printf("Join thread %d failed, status = %d\n", idx, status);
            return status;
        }
    }
    return 0;
}