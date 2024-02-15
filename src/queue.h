#ifndef QUEUE_H
#define QUEUE_H

#include <libpq-fe.h>

typedef struct Node {
    struct Node *next;
    int *client_socket;
} Node;

extern Node *head;
extern Node *tail;

void enqueue(int *client_socket);
void *dequeue();

#endif
