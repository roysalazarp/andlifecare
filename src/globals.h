#ifndef GLOBALS_H
#define GLOBALS_H

#include <libpq-fe.h>

#define REQUEST_BUFFER_SIZE 1024
#define MAX_CONNECTIONS 100
#define PORT 8080

extern PGconn *conn;

#endif
