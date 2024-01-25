#ifndef GLOBALS_H
#define GLOBALS_H

#include <libpq-fe.h>

#define REQUEST_BUFFER_SIZE 1024
#define MAX_CONNECTIONS 100
#define PORT 5000

extern PGconn *conn;

#endif
