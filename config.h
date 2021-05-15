#ifndef __TCP_APP_COMMON_H__
#define __TCP_APP_COMMON_H__

#include <stdint.h>

const int N = 8;

const int WORKERS = N / 3;
const uint32_t MESSAGES_COUNT = 5;
const int CONNECTIONS_PER_WORKER =
    (N % WORKERS == 0) ? N / WORKERS : (N / WORKERS + 1);

// const int N = 64;
// const int MESSAGES_COUNT = 5000000;

const int SERVER_PORT = 5000;
const char SERVER_IP[] = "127.0.0.1";
const int MAX_RETRIES = 5;

typedef struct {
  uint32_t msg_id;
  const char *msgmv;
} message;

#endif