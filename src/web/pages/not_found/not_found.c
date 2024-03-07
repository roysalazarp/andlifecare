#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "web/web.h"

int web_not_found(int client_socket, HttpRequest *request) {
    char response[] = "HTTP/1.1 404 Not Found\r\n"
                      "Content-Type: text/html\r\n"
                      "\r\n"
                      "<html><body><h1>404 Not Found</h1></body></html>";
    if (send(client_socket, response, strlen(response), 0) == -1) {
        fprintf(stderr, "Failed send HTTP response\nError code: %d\n", errno);
        return -1;
    }

    close(client_socket);

    return 0;
}
