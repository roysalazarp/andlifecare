#include <arpa/inet.h>
#include <errno.h>
#include <libpq-fe.h>
#include <linux/limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "globals.h"
#include "queue.h"
#include "utils/utils.h"
#include "web/web.h"

#define PRINT_MESSAGE_COLOR "#0059ff"
#define PRINT_MESSAGE_STATUS "#42ff62"

volatile sig_atomic_t keep_running = 1;

int server_socket;

PGconn *conn_pool[POOL_SIZE];
pthread_t thread_pool[POOL_SIZE];
pthread_key_t thread_index_key;
int thread_indices[POOL_SIZE];
pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thread_condition_var = PTHREAD_COND_INITIALIZER;

typedef struct {
    char DB_NAME[12];
    char DB_USER[16];
    char DB_PASSWORD[9];
    char DB_HOST[10];
    char DB_PORT[5];
} ENV;

ENV env;

void sigint_handler(int signo);
unsigned int has_file_extension(const char *file_path, const char *extension);
int setup_server_socket(int *fd);
int extract_request(char **buffer, int client_socket);
void *router(void *p_client_socket, int conn_index);
void *threads_handler(void *arg);
void print_colored_message(const char *color, const char *format, ...);
void print_banner();

int main() {
    print_banner();

    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        log_error("Failed to set up signal handler\n");
        exit(EXIT_FAILURE);
    }

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

    pthread_key_create(&thread_index_key, NULL);

    /* Spin up threads and db connection pool */
    int i;
    for (i = 0; i < POOL_SIZE; i++) {
        thread_indices[i] = i;
        pthread_create(&thread_pool[i], NULL, threads_handler, &thread_indices[i]);
        conn_pool[i] = PQconnectdbParams(db_connection_keywords, db_connection_values, 0);

        if (PQstatus(conn_pool[i]) != CONNECTION_OK) {
            log_error(PQerrorMessage(conn_pool[i]));
            exit(EXIT_FAILURE);
        }
    }

    print_colored_message(PRINT_MESSAGE_COLOR, "DB connection established: ");
    print_colored_message(PRINT_MESSAGE_STATUS, "Success!\n");

    if (setup_server_socket(&server_socket) == -1) {
        /* PQfinish(conn); */
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
            /* PQfinish(conn); */
            exit(EXIT_FAILURE);
        }

        int *p_client_socket = malloc(sizeof(int));
        *p_client_socket = client_socket;

        pthread_mutex_lock(&thread_mutex);
        enqueue(p_client_socket);
        pthread_cond_signal(&thread_condition_var);
        pthread_mutex_unlock(&thread_mutex);
    }

    return 0;
}

void *threads_handler(void *arg) {
    int *thread_index = (int *)arg;
    pthread_setspecific(thread_index_key, thread_index);

    while (1) {
        int *p_client_socket;
        pthread_mutex_lock(&thread_mutex);

        if ((p_client_socket = dequeue()) == NULL) {
            pthread_cond_wait(&thread_condition_var, &thread_mutex);
            p_client_socket = dequeue();
        }

        pthread_mutex_unlock(&thread_mutex);
        if (p_client_socket != NULL) {
            router(p_client_socket, *thread_index);
        }
    }
}

int extract_request(char **buffer, int client_socket) {
    size_t buffer_size = 1024;

    *buffer = (char *)malloc((buffer_size * (sizeof **buffer)) + 1);
    if (*buffer == NULL) {
        log_error("Failed to allocate memory for *buffer\n");
        return -1;
    }

    (*buffer)[0] = '\0';

    size_t total_bytes_received = 0;
    int bytes_received;
    while ((bytes_received = recv(client_socket, (*buffer) + total_bytes_received, buffer_size - total_bytes_received, 0)) > 0) {
        total_bytes_received += bytes_received;

        if (total_bytes_received >= buffer_size) {
            buffer_size *= 2;
            (*buffer) = realloc((*buffer), buffer_size);
            if ((*buffer) == NULL) {
                perror("realloc");
                log_error("Failed to reallocate memory for *buffer\n");
                close(server_socket);
                close(client_socket);
                free(*buffer);
                *buffer = NULL;
                return -1;
            }
        } else {
            break;
        }
    }

    if (bytes_received == -1) {
        log_error("Failed extract headers from *buffer\n");
        close(server_socket);
        close(client_socket);
        free(*buffer);
        *buffer = NULL;
        /* PQfinish(conn); */
        return -1;
    }

    (*buffer)[buffer_size] = '\0';

    return 0;
}

void *router(void *p_client_socket, int conn_index) {
    int client_socket = *((int *)p_client_socket);
    free(p_client_socket);
    p_client_socket = NULL;

    char *request = NULL;
    extract_request(&request, client_socket);

    HttpRequest parsed_http_request;
    web_utils_parse_http_request(&parsed_http_request, request);

    free(request);
    request = NULL;

    if (has_file_extension(parsed_http_request.url, ".css") == 0 && strcmp(parsed_http_request.method, "GET") == 0) {
        char response_headers[] = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: text/css\r\n"
                                  "\r\n";

        if (web_serve_static(client_socket, parsed_http_request.url, response_headers, strlen(response_headers)) == -1) {
            close(server_socket);
            close(client_socket);
            /* PQfinish(conn); */
            return NULL;
        }
    }

    if (has_file_extension(parsed_http_request.url, ".js") == 0 && strcmp(parsed_http_request.method, "GET") == 0) {
        char response_headers[] = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: application/javascript\r\n"
                                  "\r\n";

        if (web_serve_static(client_socket, parsed_http_request.url, response_headers, strlen(response_headers)) == -1) {
            close(server_socket);
            close(client_socket);
            /* PQfinish(conn); */
            return NULL;
        }
    }

    char *public_route = NULL;
    if (requested_public_route(parsed_http_request.url) == 0) {
        if (strcmp(parsed_http_request.method, "GET") == 0) {
            if (construct_public_route_file_path(&public_route, parsed_http_request.url) == -1) {
                close(server_socket);
                close(client_socket);
                free(public_route);
                public_route = NULL;
                /* PQfinish(conn); */
                return NULL;
            }

            char response_headers[] = "HTTP/1.1 200 OK\r\n"
                                      "Content-Type: text/html\r\n"
                                      "\r\n";

            if (web_serve_static(client_socket, public_route, response_headers, strlen(response_headers)) == -1) {
                close(server_socket);
                close(client_socket);
                free(public_route);
                public_route = NULL;
                /* PQfinish(conn); */
                return NULL;
            }
        }
    }

    /**
     * Routes start here
     */
    if (strcmp(parsed_http_request.url, "/") == 0) {
        if (strcmp(parsed_http_request.method, "GET") == 0) {
            if (web_page_home_get(client_socket, &parsed_http_request) == -1) {
                close(server_socket);
                close(client_socket);
                /* PQfinish(conn); */
                return NULL;
            }
        }
    } else if (strcmp(parsed_http_request.url, "/sign-up") == 0) {
        if (strcmp(parsed_http_request.method, "GET") == 0) {
            if (web_page_sign_up_get(client_socket, &parsed_http_request) == -1) {
                close(server_socket);
                close(client_socket);
                /* PQfinish(conn); */
                return NULL;
            }
        }
    } else if (strcmp(parsed_http_request.url, "/sign-up/create-user") == 0) {
        if (strcmp(parsed_http_request.method, "POST") == 0) {
            if (web_page_sign_up_create_user_post(client_socket, &parsed_http_request) == -1) {
                close(server_socket);
                close(client_socket);
                /* PQfinish(conn); */
                return NULL;
            }
        }
    } else if (strcmp(parsed_http_request.url, "/ui-test") == 0) {
        if (strcmp(parsed_http_request.method, "GET") == 0) {
            if (web_page_ui_test_get(client_socket, &parsed_http_request, conn_index) == -1) {
                close(server_socket);
                close(client_socket);
                /* PQfinish(conn); */
                return NULL;
            }
        }
    } else {
        if (web_page_not_found(client_socket, &parsed_http_request) == -1) {
            close(server_socket);
            close(client_socket);
            /* PQfinish(conn); */
            return NULL;
        }
    }

    free(public_route);
    public_route = NULL;

    web_utils_http_request_free(&parsed_http_request);

    return NULL;
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