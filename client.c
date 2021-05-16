#include "utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void *client_routine(void *args) {
    int i = 0, socket_fd = 0, retry = 0;
    int thread_idx = *(int *)args;

    int msg_len = 0;
    char msg_buf[BUFFER_SIZE];
    uint32_t ack = 0;

    struct sockaddr_in server_addr = init_server_addr();
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        // TODO proccess error
        printf("Create client socket for %d failed.\n", thread_idx);
        return NULL;
    }

    while (connect(socket_fd, (struct sockaddr *)&server_addr,
                    sizeof(server_addr)) == -1) {
        if (++retry == MAX_RETRIES) {
        // TODO proccess error
        printf("Failed to connect to server. \n");
        return NULL;
        }
        printf("[%d] Sleep and retry #%d\n", thread_idx, retry);
        sleep(1);
    }

    for (i = 0; i < MESSAGES_COUNT; ++i) {
        msg_len = prepare_msg_buf(thread_idx, i, msg_buf);
        send(socket_fd, msg_buf, msg_len, 0);
        
        readbytes(socket_fd, (char *)&ack, 4);
        ack = ntohl(ack);
        printf("id: %d, ack: %d\n", thread_idx, ack);
    }

    msg_len = prepare_closing_buf(msg_buf);
    send(socket_fd, msg_buf, msg_len, 0);
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
            // TODO proccess error
            printf("Create thread %d failed, status = %d\n", idx, status);
        }
    }

    for (idx = 0; idx < N; ++idx) {
        status = pthread_join(threads[idx], NULL);
        if (status != 0) {
            // TODO proccess error
            printf("Join thread %d failed, status = %d\n", idx, status);
        }
    }
    return 0;
}