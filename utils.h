#ifndef __TCP_APP_UTILS_H__
#define __TCP_APP_UTILS_H__

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#define N 64
#define NUM_WORKERS (N / 3)
#define SERVER_PORT 5000
#define SERVER_IP "127.0.0.1"

#define MESSAGE_SIZE 8192

#define MAX_INFLIGHT_CLIENT 1000
#define MAX_INFLIGHT_SERVER 1000

// Uncomment to enable debug output:
// #define DEBUGF printf
#define DEBUGF(...)

extern const uint32_t CLOSING_ID;

// Message struct - should be 8kbyte size.
typedef struct {
    uint32_t msg_id;
    uint32_t body_size;
    char msg[MESSAGE_SIZE - 8];
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