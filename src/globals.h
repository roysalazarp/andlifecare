#ifndef GLOBALS_H
#define GLOBALS_H

#include <libpq-fe.h>

#define REQUEST_BUFFER_SIZE 1024
#define MAX_CONNECTIONS 100
#define PORT 8080
#define POOL_SIZE 70

extern PGconn *conn_pool[POOL_SIZE];

#endif
