#include "utils.h"

#include <sys/socket.h>
#include <sys/types.h>

struct sockaddr_in init_server_addr() {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    return server_addr;
}

int recv_n_bytes(int conn_fd, size_t n, char *to) {
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

int recv_msg(int conn_fd, message_t *out) {
    char buf[HEADER_SIZE];
    if (recv_n_bytes(conn_fd, HEADER_SIZE, buf) == -1) {
        printf("Failed to read header.\n");
        return -1;
    }
    out->msg_id = ntohl(*((uint32_t *)buf));
    out->body_size = ntohl(*((uint32_t *)(buf + 4)));
    if (out->body_size > sizeof(out->payload)) {
        printf("Body size (%d) is bigger than max message size (%ld)\n", out->body_size,
               sizeof(out->payload));
        return -1;
    }

    if (recv_n_bytes(conn_fd, out->body_size, out->payload) == -1) {
        printf("Failed to read body with size %d.\n", out->body_size);
        return -1;
    }
    return 0;
}

int write_msg(int thread_idx, int msg_idx, char *out) {
    const char c = '0' + thread_idx;
    // NOTE: You can send messages with smaller body size, if needed. Example:
    //  uint32_t body_size = sprintf(out + HEADER_SIZE, "#%d: from %d.\n", msg_idx, thread_idx);
    const uint32_t body_size = BODY_SIZE;
    memset(out + HEADER_SIZE, c, body_size);

    *(uint32_t *)out = htonl(msg_idx);
    *(uint32_t *)(out + 4) = htonl(body_size);
    return body_size + HEADER_SIZE;
}

const uint32_t CLOSING_ID = -1;

int write_closing_msg(char *out) {
    *(uint32_t *)out = CLOSING_ID;  // msg_idx = CLOSING_ID
    *(uint32_t *)(out + 4) = 0;     // body_size = 0
    return HEADER_SIZE;
}
