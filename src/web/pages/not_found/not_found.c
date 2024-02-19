#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "web/web.h"

/* Reviewed: Fri 17. Feb 2024 */
int web_page_not_found(int client_socket, HttpRequest *request) {
    char response[] = "HTTP/1.1 404 Not Found\r\n"
                      "Content-Type: text/html\r\n"
                      "\r\n"
                      "<html><body><h1>404 Not Found</h1></body></html>";
    send(client_socket, response, strlen(response), 0);

    return 0;
}
