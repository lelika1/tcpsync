#include "utils.h"

#include <sys/socket.h>
#include <sys/types.h>

const int N = 8; // 64
const int WORKERS = N / 3;
const uint32_t MESSAGES_COUNT = 1000; // 5000000
const int CONNECTIONS_PER_WORKER =
    (N % WORKERS == 0) ? N / WORKERS : (N / WORKERS + 1);

const int SERVER_PORT = 5000;
const char SERVER_IP[] = "127.0.0.1";
const int MAX_RETRIES = 5;

struct sockaddr_in init_server_addr() {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    return server_addr;
}

int readbytes(int conn_fd, char *to, size_t n) {
    int got = 0;
    while (n > 0) {
        got = recv(conn_fd, to, n, 0);
        if (got == -1) {
            return -1;
        }
        to += got;
        n -= got;
    }
    return 0;
}

// Returns size of data in `out`.
int deserialize_msg(int conn_fd, message_t *out) {
    char buf[9];
    if (readbytes(conn_fd, buf, 9) == -1) {
        printf("Failed to read header.\n");
        return -1;
    }
    out->msg_id = ntohl(*((uint32_t *)buf));
    out->body_size = ntohl(*((uint32_t *)(buf+4)));
    if (out->body_size > sizeof(out->msg)) {
        printf("Body size (%d) is bigger than max message size (%ld)\n", out->body_size, sizeof(out->msg)); 
        return -1;
    }
    out->is_finish = buf[8];
    if (readbytes(conn_fd, out->msg, out->body_size) == -1) {
        printf("Failed to read body with size %d.\n", out->body_size);
        return -1;
    }
    return 0;
}

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

int prepare_msg_buf(int thread_idx, int msg_idx, char* msg_buf) {
    message_t msg;
    int msg_len;

    msg.msg_id = thread_idx;
    msg.is_finish = '0';
    msg.body_size = sprintf(msg.msg, "Hi from %d msg: %d.\n", thread_idx, msg_idx);
    msg_len = serialize_msg(&msg, msg_buf);
    return msg_len;
}

int prepare_closing_buf(char* msg_buf) {
    message_t msg;
    int msg_len;

    msg.msg_id = -1;
    msg.is_finish = '1';
    msg.body_size = 0;
    msg_len = serialize_msg(&msg, msg_buf);
    return msg_len;
}