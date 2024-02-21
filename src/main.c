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

#define MAX_CONNECTIONS 100
#define PORT 8080

#define PRINT_MESSAGE_COLOR "#0059ff"
#define PRINT_MESSAGE_STATUS "#42ff62"

volatile sig_atomic_t keep_running = 1;

int server_socket;

PGconn *conn_pool[POOL_SIZE];
pthread_t thread_pool[POOL_SIZE];
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
int read_request(char **request_buffer, int client_socket);
int router(void *p_client_socket, unsigned short conn_index);
void *thread_function(void *arg);
int print_colored_message(const char *hex_color, const char *format, ...);
void print_banner();

/* Reviewed: Fri 19. Feb 2024 */
int main() {
    int retval = 0;

    unsigned short i;

    for (i = 0; i < POOL_SIZE; i++) {
        conn_pool[i] = NULL;
    }

    print_banner();

    /**
     * Registers a signal handler for SIGINT (to terminate the process) to exit the
     * program gracefully for Valgrind to show the program report.
     */
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        log_error("Failed to set up signal handler\n");
        exit(EXIT_FAILURE);
    }

    if (load_values_from_file(&env, ".env.dev") == -1) {
        log_error("Failed to load env variables from file\n");
        retval = -1;
        goto main_cleanup;
    }

    if (setup_server_socket(&server_socket) == -1) {
        retval = -1;
        goto main_cleanup;
    }

    print_colored_message(PRINT_MESSAGE_COLOR, "Server listening on port %d: ", PORT);
    print_colored_message(PRINT_MESSAGE_STATUS, "Success!\n");

    const char *db_connection_keywords[] = {"dbname", "user", "password", "host", "port", NULL};
    const char *db_connection_values[6];
    db_connection_values[0] = env.DB_NAME;
    db_connection_values[1] = env.DB_USER;
    db_connection_values[2] = env.DB_PASSWORD;
    db_connection_values[3] = env.DB_HOST;
    db_connection_values[4] = env.DB_PORT;
    db_connection_values[5] = NULL;

    /** Create threads and db connection pool */
    for (i = 0; i < POOL_SIZE; i++) {
        unsigned short *a = malloc(sizeof(unsigned short));
        *a = i;
        if (pthread_create(&thread_pool[*a], NULL, &thread_function, a) != 0) {
            log_error("Failed to create thread\n");
            retval = -1;
            goto main_cleanup;
        }
    }

    for (i = 0; i < POOL_SIZE; i++) {
        conn_pool[i] = PQconnectdbParams(db_connection_keywords, db_connection_values, 0);
        if (PQstatus(conn_pool[i]) != CONNECTION_OK) {
            log_error("Failed to create db connection pool\n");
            retval = -1;
            goto main_cleanup;
        }
    }

    print_colored_message(PRINT_MESSAGE_COLOR, "DB connection established: ");
    print_colored_message(PRINT_MESSAGE_STATUS, "Success!\n");

    while (keep_running) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof client_addr;

        /** The while loop will wait at accept for a new client to connect */
        int client_socket;
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1 && keep_running == 0) {
            pthread_cond_signal(&thread_condition_var);
            retval = 0;
            goto main_cleanup;
        }

        if (client_socket == -1) {
            log_error("Failed to create client socket\n");
            retval = -1;
            goto main_cleanup;
        }

        int *p_c_s = malloc(sizeof(int));
        *p_c_s = client_socket;

        if (pthread_mutex_lock(&thread_mutex) != 0) {
            log_error("Failed to lock mutex\n");
            retval = -1;
            goto main_cleanup;
        }

        printf("- New request: enqueue fd %d\n", *p_c_s);
        enqueue(p_c_s);

        if (pthread_cond_signal(&thread_condition_var) != 0) {
            log_error("Failed to unlock mutex\n");
            retval = -1;
            goto main_cleanup;
        }

        if (pthread_mutex_unlock(&thread_mutex) != 0) {
            log_error("Failed to unlock mutex\n");
            retval = -1;
            goto main_cleanup;
        }
    }

main_cleanup:

    for (i = 0; i < POOL_SIZE; i++) {
        PQfinish(conn_pool[i]);
    }

    close(server_socket);

    for (i = 0; i < POOL_SIZE; i++) {
        pthread_cond_signal(&thread_condition_var);
    }

    for (i = 0; i < POOL_SIZE; i++) {
        printf("(Thread %d) Cleaning up thread %lu\n", i, thread_pool[i]);

        if (pthread_join(thread_pool[i], NULL) != 0) {
            printf("Something wrong with pthread_join\n");
        }
    }

    return retval;
}

void *thread_function(void *arg) {
    unsigned short *thread_index = (unsigned short *)arg;
    pthread_t tid = pthread_self();
    printf("(Thread %d) Setting up thread %lu\n", *thread_index, tid);

    while (1) {
        int *p_client_socket;

        if (pthread_mutex_lock(&thread_mutex) != 0) {
            /** TODO: cleanup */
        }

        if ((p_client_socket = dequeue()) == NULL) {
            pthread_cond_wait(&thread_condition_var, &thread_mutex);
            if (keep_running == 0) {
                pthread_mutex_unlock(&thread_mutex);
                goto out;
            }

            p_client_socket = dequeue();
            printf("(Thread %d) Dequeueing(received signal)...\n", *thread_index);
            goto skip_print;
        }

        printf("(Thread %d) Dequeueing...\n", *thread_index);

    skip_print:

        if (pthread_mutex_unlock(&thread_mutex) != 0) {
            /** TODO: cleanup */
        }

        if (p_client_socket != NULL && keep_running == 1) {
            if (router(p_client_socket, *thread_index) == -1) {
                printf("error at route!\n");
                break;
            }
        }
    }

out:
    printf("(Thread %d) Out of while loop\n", *thread_index);

    free(arg);
    arg = NULL;

    return NULL;
}

int router(void *p_client_socket, unsigned short conn_index) {
    int retval = 0;

    int client_socket = *((int *)p_client_socket);
    free(p_client_socket);
    p_client_socket = NULL;

    char *request = NULL;
    if (read_request(&request, client_socket) == -1) {
        retval = -1;
        goto socket_cleanup;
    }

    if (strlen(request) == 0) {
        log_error("Request is empty\n");
        retval = 0;
        goto cleanup_request_buffer;
    }

    HttpRequest parsed_http_request;
    if (web_utils_parse_http_request(&parsed_http_request, request) == -1) {
        retval = -1;
        goto cleanup_request_buffer;
    }

    if (has_file_extension(parsed_http_request.url, ".css") == 0 && strcmp(parsed_http_request.method, "GET") == 0) {
        /** TODO: improve http response headers */
        char response_headers[] = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: text/css\r\n"
                                  "\r\n";

        if (web_serve_static(client_socket, parsed_http_request.url, response_headers, strlen(response_headers)) == -1) {
            retval = -1;
            goto cleanup_parsed_request;
        }
    }

    if (has_file_extension(parsed_http_request.url, ".js") == 0 && strcmp(parsed_http_request.method, "GET") == 0) {
        /** TODO: improve http response headers */
        char response_headers[] = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: application/javascript\r\n"
                                  "\r\n";

        if (web_serve_static(client_socket, parsed_http_request.url, response_headers, strlen(response_headers)) == -1) {
            retval = -1;
            goto cleanup_parsed_request;
        }
    }

    char *public_route = NULL;
    if (requested_public_route(parsed_http_request.url) == 0) {
        if (strcmp(parsed_http_request.method, "GET") == 0) {
            if (construct_public_route_file_path(&public_route, parsed_http_request.url) == -1) {
                retval = -1;
                goto cleanup;
            }

            /** TODO: improve http response headers */
            char response_headers[] = "HTTP/1.1 200 OK\r\n"
                                      "Content-Type: text/html\r\n"
                                      "\r\n";

            if (web_serve_static(client_socket, public_route, response_headers, strlen(response_headers)) == -1) {
                retval = -1;
                goto cleanup;
            }
        }
    }

    /**
     * Routes start here
     */
    if (strcmp(parsed_http_request.url, "/") == 0) {
        if (strcmp(parsed_http_request.method, "GET") == 0) {
            if (web_page_home_get(client_socket, &parsed_http_request) == -1) {
                retval = -1;
                goto cleanup;
            }
        }
    } else if (strcmp(parsed_http_request.url, "/sign-up") == 0) {
        if (strcmp(parsed_http_request.method, "GET") == 0) {
            if (web_page_sign_up_get(client_socket, &parsed_http_request) == -1) {
                retval = -1;
                goto cleanup;
            }
        }
    } else if (strcmp(parsed_http_request.url, "/sign-up/create-user") == 0) {
        if (strcmp(parsed_http_request.method, "POST") == 0) {
            if (web_page_sign_up_create_user_post(client_socket, &parsed_http_request) == -1) {
                retval = -1;
                goto cleanup;
            }
        }
    } else if (strcmp(parsed_http_request.url, "/ui-test") == 0) {
        if (strcmp(parsed_http_request.method, "GET") == 0) {
            if (web_page_ui_test_get(client_socket, &parsed_http_request, conn_index) == -1) {
                retval = -1;
                goto cleanup;
            }
        }
    } else {
        if (web_page_not_found(client_socket, &parsed_http_request) == -1) {
            retval = -1;
            goto cleanup;
        }
    }

cleanup:
    free(public_route);
    public_route = NULL;

cleanup_parsed_request:
    web_utils_http_request_free(&parsed_http_request);

cleanup_request_buffer:
    free(request);
    request = NULL;

socket_cleanup:
    close(client_socket);

    return retval;
}

/* Reviewed: Fri 17. Feb 2024 */
int read_request(char **request_buffer, int client_socket) {
    size_t buffer_size = 1024;

    *request_buffer = (char *)malloc((buffer_size * (sizeof **request_buffer)) + 1);
    if (*request_buffer == NULL) {
        log_error("Failed to allocate memory for *request_buffer\n");
        return -1;
    }

    (*request_buffer)[0] = '\0';

    size_t chunk_read = 0;
    int bytes_read;
    while ((bytes_read = recv(client_socket, (*request_buffer) + chunk_read, buffer_size - chunk_read, 0)) > 0) {
        chunk_read += bytes_read;

        if (chunk_read >= buffer_size) {
            buffer_size *= 2;
            (*request_buffer) = realloc((*request_buffer), buffer_size);
            if ((*request_buffer) == NULL) {
                log_error("Failed to reallocate memory for *request_buffer\n");
                free(*request_buffer);
                *request_buffer = NULL;
                return -1;
            }
        } else {
            break;
        }
    }

    if (bytes_read == -1) {
        log_error("Failed extract headers from *request_buffer\n");
        free(*request_buffer);
        *request_buffer = NULL;
        return -1;
    }

    (*request_buffer)[buffer_size] = '\0';

    return 0;
}

/* Reviewed: Fri 17. Feb 2024 */
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

/* Reviewed: Fri 16. Feb 2024 */
void sigint_handler(int signo) {
    if (signo == SIGINT) {
        printf("\nReceived SIGINT, exiting program...\n");
        keep_running = 0;
    }
}

/* Reviewed: Fri 17. Feb 2024 */
int print_colored_message(const char *hex_color, const char *format, ...) {
    /** Ensure the hex_color starts with '#' and extract decimal values */
    if (hex_color[0] != '#' || strlen(hex_color) != 7) {
        printf("Invalid color format. Please use the format '#RRGGBB'.\n");
        return -1;
    }

    unsigned int r, g, b;
    sscanf(hex_color + 1, "%2x%2x%2x", &r, &g, &b);

    /** Print message with specified color and formatting */
    printf("\033[0;38;2;%d;%d;%dm", r, g, b);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\033[0m");

    return 0;
}

/* Reviewed: Fri 17. Feb 2024 */
int setup_server_socket(int *fd) {
    *fd = socket(AF_INET, SOCK_STREAM, 0);

    if (*fd == -1) {
        log_error("Failed to create server socket\n");
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
        log_error("Failed to bind server socker to address and port\n");
        close(*fd);
        return -1;
    }

    if (listen(*fd, MAX_CONNECTIONS) == -1) {
        log_error("Failed to set up server socker to listen for incoming connections\n");
        close(*fd);
        return -1;
    }

    return 0;
}

/* Reviewed: Fri 16. Feb 2024 */
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