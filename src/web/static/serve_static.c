#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/limits.h>
#include <string.h>

#include "utils/utils.h"
#include "globals.h"

int serve_static(int client_socket, char *path, const char *response_headers, size_t response_headers_length) {
    char *file_path;
    file_path = (char*)malloc(PATH_MAX * (sizeof *file_path) + 1);
    if (file_path == NULL) {
        log_error("Failed to allocate memory for file_path\n");
        return -1;
    }

    file_path[0] = '\0';

    if (sprintf(file_path, "%s%s", CWD, path) < 0) {
        log_error("Formatted string truncated\n");
        return -1;
    }

    long file_length = calculate_file_bytes_length(file_path);
    if (file_length == -1) {
        free(file_path);
        file_path = NULL;
        return -1;
    }

    char *response;
    size_t response_length = ((size_t)(file_length)) + response_headers_length;
    response = (char*)malloc(response_length * (sizeof *response) + 1);
    if (response == NULL) {
        log_error("Failed to allocate memory for response\n");
        free(file_path);
        file_path = NULL;
        return -1;
    }

    response[0] = '\0';

    strcpy(response, response_headers);

    if (read_file(response + response_headers_length, file_path, file_length) == -1) {
        free(file_path);
        file_path = NULL;
        free(response);
        response = NULL;
        return -1;
    }

    response[response_length] = '\0';

    free(file_path);
    file_path = NULL;

    /* Send the HTTP response */
    if (send(client_socket, response, response_length, 0) == -1) {
        log_error("Failed send HTTP response\n");
        free(response);
        response = NULL;
        return -1;
    }

    free(response);
    response = NULL;

    close(client_socket);
    return 0;
}