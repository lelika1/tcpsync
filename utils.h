#ifndef __TCP_APP_UTILS_H__
#define __TCP_APP_UTILS_H__

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

extern const int N;
extern const int WORKERS;
extern const uint32_t MESSAGES_COUNT;
extern const int CONNECTIONS_PER_WORKER;
extern const int SERVER_PORT;
extern const char SERVER_IP[];
extern const int MAX_RETRIES;

#define BUFFER_SIZE 8192

typedef struct {
  uint32_t msg_id;
  uint32_t body_size;
  char is_finish;
  char msg[8192];
} message_t;

int readbytes(int conn_fd, char *to, size_t n);

// Returns size of data in `out`.
int deserialize_msg(int conn_fd, message_t *out);

// Returns size of data in `out`.
int serialize_msg(const message_t *msg, char *out);

int prepare_msg_buf(int thread_idx, int msg_idx, char *msg_buf);
int prepare_closing_buf(char *msg_buf);

struct sockaddr_in init_server_addr();

#endif