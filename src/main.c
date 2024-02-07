#include <arpa/inet.h>
#include <errno.h>
#include <libpq-fe.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "globals.h"
#include "utils/utils.h"
#include "web/web.h"

#define PRINT_MESSAGE_COLOR "#0059ff"
#define PRINT_MESSAGE_STATUS "#42ff62"

volatile sig_atomic_t keep_running = 1;

PGconn *conn;

typedef struct {
    char DB_NAME[12];
    char DB_USER[16];
    char DB_PASSWORD[9];
    char DB_HOST[10];
    char DB_PORT[5];
} ENV;

void sigint_handler(int signo);
unsigned int has_file_extension(const char *file_path, const char *extension);
int setup_server_socket(int *fd);
void print_colored_message(const char *color, const char *format, ...);
void print_banner();

int main() {
    print_banner();

    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        log_error("Failed to set up signal handler\n");
        exit(EXIT_FAILURE);
    }

    ENV env;

    if (load_values_from_file(&env, ".env.dev") == -1) {
        log_error("Failed to load env variables from file\n");
        exit(EXIT_FAILURE);
    }

    const char *db_connection_keywords[] = {"dbname", "user", "password", "host", "port", NULL};
    const char *db_connection_values[6];
    db_connection_values[0] = env.DB_NAME;
    db_connection_values[1] = env.DB_USER;
    db_connection_values[2] = env.DB_PASSWORD;
    db_connection_values[3] = env.DB_HOST;
    db_connection_values[4] = env.DB_PORT;
    db_connection_values[5] = NULL;

    conn = PQconnectdbParams(db_connection_keywords, db_connection_values, 0);

    if (PQstatus(conn) != CONNECTION_OK) {
        log_error(PQerrorMessage(conn));
        exit(EXIT_FAILURE);
    }

    print_colored_message(PRINT_MESSAGE_COLOR, "DB connection established: ");
    print_colored_message(PRINT_MESSAGE_STATUS, "Success!\n");

    int server_socket;
    if (setup_server_socket(&server_socket) == -1) {
        PQfinish(conn);
        exit(EXIT_FAILURE);
    }

    print_colored_message(PRINT_MESSAGE_COLOR, "Server listening on port %d: ", PORT);
    print_colored_message(PRINT_MESSAGE_STATUS, "Success!\n");

    while (keep_running) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof client_addr;

        int client_socket;
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            log_error("Failed to create client socket\n");
            close(server_socket);
            PQfinish(conn);
            exit(EXIT_FAILURE);
        }

        /**
         * TODO: realloc when request buffer is not large enough
         */
        char *request;
        request = (char *)malloc((REQUEST_BUFFER_SIZE * (sizeof *request)) + 1);
        if (request == NULL) {
            log_error("Failed to allocate memory for request\n");
            close(server_socket);
            close(client_socket);
            PQfinish(conn);
            exit(EXIT_FAILURE);
        }

        request[0] = '\0';

        if (recv(client_socket, request, REQUEST_BUFFER_SIZE, 0) == -1) {
            log_error("Failed extract headers from request\n");
            close(server_socket);
            close(client_socket);
            free(request);
            request = NULL;
            PQfinish(conn);
            exit(EXIT_FAILURE);
        }

        if (strlen(request) == (size_t)0) {
            /**
             * For some reason sometimes the browser sends an empty request.
             * https://stackoverflow.com/questions/65386563/browser-sending-empty-request-to-own-server
             */
            printf("Received empty request\n");
            continue;
        }

        request[REQUEST_BUFFER_SIZE] = '\0';

        char *method_start_position = request;
        char *method_end_position = strchr(request, ' ');

        /* Skip one space after the method and should be the beginning of the url */
        char *url_start_position = method_end_position + 1;
        char *url_end_position = strchr(url_start_position, ' ');

        size_t method_length = method_end_position - method_start_position;

        /**
         * HTTP method will never be longer than 7 characters
         */
        char method[8];

        /**
         * Request headers -> request line -> url will refer to src/web/pages (relatively short url)
         * or pages partial updates (may be a long url)
         */
        size_t url_length = url_end_position - url_start_position;
        char *url;
        url = malloc(url_length * (sizeof *url) + 1);
        if (url == NULL) {
            log_error("Failed to allocate memory for method, url or protocol\n");
            close(server_socket);
            close(client_socket);
            free(request);
            request = NULL;
            PQfinish(conn);
            exit(EXIT_FAILURE);
        }

        strncpy(method, request, method_length);
        method[method_length] = '\0';

        strncpy(url, url_start_position, url_length);
        url[url_length] = '\0';

        char *start_of_params = strchr(url, '?');

        if (start_of_params != NULL) {
            *start_of_params = '\0';
        }

        if (has_file_extension(url, ".css") == 0 && strcmp(method, "GET") == 0) {
            char response_headers[] = "HTTP/1.1 200 OK\r\n"
                                      "Content-Type: text/css\r\n"
                                      "\r\n";

            if (web_serve_static(client_socket, url, response_headers, strlen(response_headers)) == -1) {
                close(server_socket);
                close(client_socket);
                free(request);
                request = NULL;
                free(url);
                url = NULL;
                PQfinish(conn);
                exit(EXIT_FAILURE);
            }
        }

        if (has_file_extension(url, ".js") == 0 && strcmp(method, "GET") == 0) {
            char response_headers[] = "HTTP/1.1 200 OK\r\n"
                                      "Content-Type: application/javascript\r\n"
                                      "\r\n";

            if (web_serve_static(client_socket, url, response_headers, strlen(response_headers)) == -1) {
                close(server_socket);
                close(client_socket);
                free(request);
                request = NULL;
                free(url);
                url = NULL;
                PQfinish(conn);
                exit(EXIT_FAILURE);
            }
        }

        if (strcmp(url, "/") == 0) {
            if (strcmp(method, "GET") == 0) {
                if (web_page_home_get(client_socket, request) == -1) {
                    close(server_socket);
                    close(client_socket);
                    free(request);
                    request = NULL;
                    free(url);
                    url = NULL;
                    PQfinish(conn);
                    exit(EXIT_FAILURE);
                }
            }
        } else if (strcmp(url, "/ui-test") == 0) {
            if (strcmp(method, "GET") == 0) {
                if (web_page_ui_test_get(client_socket, request) == -1) {
                    close(server_socket);
                    close(client_socket);
                    free(request);
                    request = NULL;
                    free(url);
                    url = NULL;
                    PQfinish(conn);
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            if (web_page_not_found(client_socket, request) == -1) {
                close(server_socket);
                close(client_socket);
                free(request);
                request = NULL;
                free(url);
                url = NULL;
                PQfinish(conn);
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

void sigint_handler(int signo) {
    /**
     * SIGINT (Ctrl+C) for graceful server exit, required by valgrind to report memory leaks.
     */
    if (signo == SIGINT) {
        printf("\nReceived SIGINT, exiting...\n");
        keep_running = 0;
    }
}

void print_colored_message(const char *color, const char *format, ...) {
    /* Ensure the hex color starts with '#' and extract decimal values */
    if (color[0] != '#' || strlen(color) != 7) {
        printf("Invalid color format. Please use the format '#RRGGBB'.\n");
        exit(EXIT_FAILURE);
    }

    unsigned int r, g, b;
    sscanf(color + 1, "%2x%2x%2x", &r, &g, &b);

    /* Print message with specified color and formatting */
    printf("\033[0;38;2;%d;%d;%dm", r, g, b);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\033[0m");
}

int setup_server_socket(int *fd) {
    *fd = socket(AF_INET, SOCK_STREAM, 0);

    if (*fd == -1) {
        log_error("Failed to create server *fd\n");
        return -1;
    }

    int optname = 1;
    if (setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &optname, sizeof(int)) == -1) {
        log_error("Failed to set local address for immediately reuse upon socker closed\n");
        close(*fd);
        return -1;
    }

    struct sockaddr_in server_addr;

    /* IPv4 */
    server_addr.sin_family = AF_INET;

    /**
     * Convert the port number from host byte order to network byte order (big-endian)
     */
    server_addr.sin_port = htons(PORT);

    /*
     * Listen on all available network interfaces (IPv4 addresses)
     */
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(*fd, (struct sockaddr *)&server_addr, sizeof server_addr) == -1) {
        log_error("Failed to bind *fd to address and port\n");
        close(*fd);
        return -1;
    }

    if (listen(*fd, MAX_CONNECTIONS) == -1) {
        log_error("Failed to set up *fd to listen for incoming connections\n");
        close(*fd);
        return -1;
    }

    return 0;
}

void print_banner() {
    /*
    print_colored_message("#ff5100", "▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃\n");
    printf("\n");
    print_colored_message("#ff5100", " █████╗ ███╗   ██╗██████╗ ██╗     ██╗███████╗███████╗ ██████╗ █████╗ ██████╗ ███████╗\n");
    print_colored_message("#ff5100", "██╔══██╗████╗  ██║██╔══██╗██║     ██║██╔════╝██╔════╝██╔════╝██╔══██╗██╔══██╗██╔════╝\n");
    print_colored_message("#ff5100", "███████║██╔██╗ ██║██║  ██║██║     ██║█████╗  █████╗  ██║     ███████║██████╔╝█████╗  \n");
    print_colored_message("#ff5100", "██╔══██║██║╚██╗██║██║  ██║██║     ██║██╔══╝  ██╔══╝  ██║     ██╔══██║██╔══██╗██╔══╝  \n");
    print_colored_message("#ff5100", "██║  ██║██║ ╚████║██████╔╝███████╗██║██║     ███████╗╚██████╗██║  ██║██║  ██║███████╗\n");
    print_colored_message("#ff5100", "╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝╚═╝     ╚══════╝ ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝\n");
    print_colored_message("#ff5100", "▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃\n");
    print_colored_message("#ff5100", "░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    */
    printf("\n");
}