#ifndef __TCP_APP_SERVER_H__
#define __TCP_APP_SERVER_H__

#include <stdio.h>

typedef struct {
    int conn_fd;
    FILE *out_fd;
} conn_descr_t;

typedef struct {
  int idx;
  conn_descr_t *connections;
  int conn_count;
} init_args_t;



#endif