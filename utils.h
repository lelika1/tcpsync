#ifndef __TCP_APP_UTILS_H__
#define __TCP_APP_UTILS_H__

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#define N 64
#define NUM_WORKERS (N / 3)
#define SERVER_PORT 5000
#define SERVER_IP "127.0.0.1"

// Number of messages every client (or server) thread retains in memory unacked.
// Invariant: MAX_INFLIGHT_CLIENT >= MAX_INFLIGHT_SERVER
#define MAX_INFLIGHT_CLIENT 2000
#define MAX_INFLIGHT_SERVER 2000

// Uncomment to enable debug output:
// #define DEBUGF printf
#define DEBUGF(...)

// Special ID value for the last message in client-server session.
extern const uint32_t CLOSING_ID;

#define MESSAGE_SIZE 8192
#define HEADER_SIZE 8
#define BODY_SIZE (MESSAGE_SIZE - HEADER_SIZE)

// Message struct - should be 8kbyte size.
typedef struct {
    uint32_t msg_id;
    uint32_t body_size;
    char payload[BODY_SIZE];
} message_t;

// Read from conn_fd until we've successfully read n bytes.
// Returns 0 on success and -1 on error.
int recv_n_bytes(int conn_fd, size_t n, char *to);

// Returns 0 on success and -1 on error.
int recv_msg(int conn_fd, message_t *out);

// Returns size of data written into `out`.
int write_msg(int thread_idx, int msg_idx, char *out);

// Returns size of data written into `out`.
int write_closing_msg(char *out);

struct sockaddr_in init_server_addr();

#endif