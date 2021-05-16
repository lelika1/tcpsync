#ifndef __TCP_APP_UTILS_H__
#define __TCP_APP_UTILS_H__

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

extern const int N;
extern const int NUM_WORKERS;
extern const int CONNECTIONS_PER_WORKER;
extern const int SERVER_PORT;
extern const char SERVER_IP[];

#define BUFFER_SIZE 8192

// Message struct - should be 8kbyte size.
typedef struct {
    uint32_t msg_id;
    uint32_t body_size;
    char is_finish;
    char msg[BUFFER_SIZE - 9];
} message_t;

// Read from conn_fd until we've successfully read n bytes.
int read_n_bytes(int conn_fd, size_t n, char *to);

// Returns size of data in `out`.
int read_msg(int conn_fd, message_t *out);

// Returns size of data in `out`.
int prepare_msg_buf(int thread_idx, int msg_idx, char *out);

// Returns size of data in `out`.
int prepare_closing_buf(char *out);

struct sockaddr_in init_server_addr();

#endif