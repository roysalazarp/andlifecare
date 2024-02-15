#include <stdlib.h>

#include "queue.h"

Node *head = NULL;
Node *tail = NULL;

void enqueue(int *client_socket) {
    Node *new_node = malloc(sizeof(Node));
    new_node->client_socket = client_socket;
    new_node->next = NULL;

    if (tail == NULL) {
        head = new_node;
    } else {
        tail->next = new_node;
    }

    tail = new_node;
}

void *dequeue() {
    if (head == NULL) {
        return NULL;
    }

    int *result = head->client_socket;
    Node *temp = head;
    head = head->next;

    if (head == NULL) {
        tail = NULL;
    }

    free(temp);
    temp = NULL;

    return result;
}
