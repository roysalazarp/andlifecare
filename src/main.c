#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>

#include "utils/utils.h"
#include "web/request_handlers.h"
#include "globals.h"

volatile sig_atomic_t keep_running = 1;
char *request;
char *url;
int server_socket;
int client_socket;

void sigint_handler(int signo) {
    /**
     * SIGINT (Ctrl+C) for graceful server exit, required by valgrind to report memory leaks.
     */
    if (signo == SIGINT) {
        printf("\nReceived SIGINT, exiting...\n");
        close(server_socket);
        close(client_socket);
        free(url);
        url = NULL;
        free(request);
        request = NULL;
        keep_running = 0;
    }
}

/**
 * @return 0 if the file path has the specified extension, 1 otherwise.
 */
unsigned int has_file_extension(const char *file_path, const char *extension) {
    size_t extension_length = strlen(extension) + 1;
    size_t path_length = strlen(file_path) + 1;
    
    if (extension_length > path_length) {
        return 1;
    }

    if (strncmp(file_path + path_length - extension_length, extension, extension_length) == 0) {
        return 0;
    }
    
    return 1;
}

int main() {
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        log_error("Failed to set up signal handler\n");
        exit(EXIT_FAILURE);
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1) {
        log_error("Failed to create server socket\n");
        exit(EXIT_FAILURE);
    }

    int optname = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optname, sizeof(int)) == -1) {
        log_error("Failed to set local address for immediately reuse upon socker closed\n");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    
    server_addr.sin_family = AF_INET;              /* IPv4 */
    server_addr.sin_port = htons(PORT);            /* Convert the port number from host byte order to network byte order (big-endian) */
    server_addr.sin_addr.s_addr = INADDR_ANY;      /* Listen on all available network interfaces (IPv4 addresses) */

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof server_addr) == -1) {
        log_error("Failed to bind socket to address and port\n");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CONNECTIONS) == -1) {
        log_error("Failed to set up socket to listen for incoming connections\n");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port: %d...\n", PORT);
    
    while (keep_running) {        
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof client_addr;

        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            log_error("Failed to create client socket\n");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        /** 
         * TODO: realloc when request buffer is not large enough
         */
        request = (char*)malloc((REQUEST_BUFFER_SIZE * (sizeof *request)) + 1);
        if (request == NULL) {
            log_error("Failed to allocate memory for request\n");
            close(server_socket);
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        
        request[0] = '\0';

        if (recv(client_socket, request, REQUEST_BUFFER_SIZE, 0) == -1) {
            log_error("Failed extract headers from request\n");
            close(server_socket);
            close(client_socket);
            free(request);
            request = NULL;
            exit(EXIT_FAILURE);
        }

        request[REQUEST_BUFFER_SIZE] = '\0';

        const char *first_space = strchr(request, ' ');
        const char *second_space = strchr(first_space + 1, ' ');

        size_t method_length = first_space - request;
        /**
         * HTTP method will never be longer than 7 characters
         */
        char method[8];
        
        /**
         * Request headers -> request line -> url will refer to src/web/pages (relatively short url) or pages partial updates (may be a long url)
         */
        size_t url_length = second_space - (first_space + 1);
        url = malloc(url_length * (sizeof *url) + 1);

        if (url == NULL) {
            log_error("Failed to allocate memory for method, url or protocol\n");
            close(server_socket);
            close(client_socket);
            free(request);
            request = NULL;
            exit(EXIT_FAILURE);
        }

        strncpy(method, request, method_length);
        method[method_length] = '\0';

        strncpy(url, first_space + 1, url_length);
        url[url_length] = '\0';

        if (has_file_extension(url, ".css") == 0 && strcmp(method, "GET") == 0) {
            char response_headers[] =   "HTTP/1.1 200 OK\r\n"
                                        "Content-Type: text/css\r\n"
                                        "\r\n";

            if (serve_static(client_socket, url, response_headers, strlen(response_headers)) == -1) {
                close(server_socket);
                close(client_socket);
                free(request);
                request = NULL;
                free(url);
                url = NULL;
                exit(EXIT_FAILURE);
            }
        }

        if (has_file_extension(url, ".js") == 0 && strcmp(method, "GET") == 0) {
            char response_headers[] =   "HTTP/1.1 200 OK\r\n"
                                        "Content-Type: application/javascript\r\n"
                                        "\r\n";

            if (serve_static(client_socket, url, response_headers, strlen(response_headers)) == -1) {
                close(server_socket);
                close(client_socket);
                free(request);
                request = NULL;
                free(url);
                url = NULL;
                exit(EXIT_FAILURE);
            }
        }


        if (strcmp(url, "/") == 0) {
            if (strcmp(method, "GET") == 0) {
                if (home_get(client_socket, request) == -1) {
                    close(server_socket);
                    close(client_socket);
                    free(request);
                    request = NULL;
                    free(url);
                    url = NULL;
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            if (not_found(client_socket, request) == -1) {
                close(server_socket);
                close(client_socket);
                free(request);
                request = NULL;
                free(url);
                url = NULL;
                exit(EXIT_FAILURE);
            }
        }

        free(url);
        url = NULL;
        free(request);
        request = NULL;

    }

    return 0;
}
