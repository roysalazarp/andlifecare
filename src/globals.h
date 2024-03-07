#ifndef GLOBALS_H
#define GLOBALS_H

#include <libpq-fe.h>

#define POOL_SIZE 3
extern PGconn *conn_pool[POOL_SIZE];

#endif
