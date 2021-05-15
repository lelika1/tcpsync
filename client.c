#include "config.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Returns size of data in `out`.
int serialize_msg(const message_t *msg, char *out) {
    *((uint32_t*)out) = htonl(msg->msg_id);
    out += 4;
    *((uint32_t *)out) = htonl(msg->body_size);
    out += 4;
    *out = msg->is_finish;
    out += 1;
    memcpy(out, msg->msg, msg->body_size);
    return msg->body_size + 9;
}

void *client_routine(void *args) {
    int i = 0, socket_fd = 0, retry = 0;
    message_t msg;
    int thread_idx = *(int *)args;

    int msg_len = 0;
    char msg_buf[BUFFER_SIZE];

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
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

    msg.msg_id = 1;
    msg.body_size = sprintf(msg.msg, "Hi from %d", thread_idx);
    msg_len = serialize_msg(&msg, msg_buf);
    send(socket_fd, msg_buf, msg_len, 0);

    for (i = 0; i < MESSAGES_COUNT; ++i) {
    }
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