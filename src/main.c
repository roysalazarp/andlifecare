#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <linux/limits.h>
#include <libpq-fe.h>

#include "utils/utils.h"
#include "web/request_handlers.h"
#include "globals.h"

volatile sig_atomic_t keep_running = 1;
char *request;
char *url;
int server_socket;
int client_socket;

PGconn *conn;

typedef struct {
    char DB_NAME[12];
    char DB_USER[16];
    char DB_PASSWORD[9];
    char DB_HOST[10];
    char DB_PORT[5];
} ENV;

void sigint_handler(int signo) {
    /**
     * SIGINT (Ctrl+C) for graceful server exit, required by valgrind to report memory leaks.
     */
    if (signo == SIGINT) {
        printf("\nReceived SIGINT, exiting...\n");
        PQfinish(conn);
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

    char *env_path;
    env_path = (char*)malloc(PATH_MAX * (sizeof *env_path) + 1);
    if (env_path == NULL) {
        log_error("Failed to allocate memory for env_path\n");
        return -1;
    }
    
    env_path[0] = '\0';

    if (build_absolute_path(env_path, "/.env.dev") == -1) {
        free(env_path);
        env_path = NULL;
        return -1;
    }

    ENV env;

    if (load_values_from_file(&env, env_path) == -1) {
        free(env_path);
        env_path = NULL;
        log_error("Failed to load env variables from file\n");
        exit(EXIT_FAILURE);
    }

    free(env_path);
    env_path = NULL;

    const char *values[5];
    values[0] = env.DB_NAME;
    values[1] = env.DB_USER;
    values[2] = env.DB_PASSWORD;
    values[3] = env.DB_HOST;
    values[4] = env.DB_PORT;

    const char *keywords[] = { "dbname", "user", "password", "host", "port", NULL };
    conn = PQconnectdbParams(keywords, values, 0);
        
    if (PQstatus(conn) != CONNECTION_OK) {
        log_error(PQerrorMessage(conn));
        exit(EXIT_FAILURE);
    }

    printf("DB connection established successfully!\n");

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1) {
        log_error("Failed to create server socket\n");
        PQfinish(conn);
        exit(EXIT_FAILURE);
    }

    int optname = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optname, sizeof(int)) == -1) {
        log_error("Failed to set local address for immediately reuse upon socker closed\n");
        PQfinish(conn);
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    
    server_addr.sin_family = AF_INET;              /* IPv4 */
    server_addr.sin_port = htons(PORT);            /* Convert the port number from host byte order to network byte order (big-endian) */
    server_addr.sin_addr.s_addr = INADDR_ANY;      /* Listen on all available network interfaces (IPv4 addresses) */

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof server_addr) == -1) {
        log_error("Failed to bind socket to address and port\n");
        PQfinish(conn);
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CONNECTIONS) == -1) {
        log_error("Failed to set up socket to listen for incoming connections\n");
        PQfinish(conn);
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
            PQfinish(conn);
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        /** 
         * TODO: realloc when request buffer is not large enough
         */
        request = (char*)malloc((REQUEST_BUFFER_SIZE * (sizeof *request)) + 1);
        if (request == NULL) {
            log_error("Failed to allocate memory for request\n");
            PQfinish(conn);
            close(server_socket);
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        
        request[0] = '\0';

        if (recv(client_socket, request, REQUEST_BUFFER_SIZE, 0) == -1) {
            log_error("Failed extract headers from request\n");
            PQfinish(conn);
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
            PQfinish(conn);
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
                PQfinish(conn);
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
                PQfinish(conn);
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
                    PQfinish(conn);
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
                PQfinish(conn);
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
