#include "config.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    int idx;
} init_args_t;

void *client_routine(void *args) {
    int idx = 0, socket_fd = 0, retry = 0;
    init_args_t* descr = (init_args_t *) args;
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        // TODO proccess error
        printf("Create client socket for %d failed.\n", idx);
        return NULL;
    }

    while (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        if (++retry == MAX_RETRIES) {
            // TODO proccess error
            printf("Failed to connect to server. \n");
            return NULL;
        }
        printf("[%d] Sleep and retry #%d\n", descr->idx, retry);
        sleep(1);
    }

    for (idx = 0; idx < MESSAGES_COUNT; ++idx) {

    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int idx = 0;
    int status = 0;
    pthread_t threads[N];
    init_args_t thread_args[N];

    for (idx = 0; idx < N; ++idx) {
        thread_args[idx].idx = idx;
        status = pthread_create(&threads[idx], NULL, client_routine, &thread_args[idx]);
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