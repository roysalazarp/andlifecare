#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int not_found(int client_socket, char *request) {
    char response_headers[] =   "HTTP/1.1 404 Not Found\r\n"
                                "Content-Type: text/html\r\n"
                                "\r\n"
                                "<html><body><h1>404 Not Found</h1></body></html>";
    send(client_socket, response_headers, strlen(response_headers), 0);
    close(client_socket);

    return 0;
}
