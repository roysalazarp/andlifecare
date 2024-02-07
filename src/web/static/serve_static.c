#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "globals.h"
#include "utils/utils.h"

int web_serve_static(int client_socket, char *path, const char *response_headers, size_t response_headers_length) {
    char *file_path;
    file_path = (char *)malloc(PATH_MAX * (sizeof *file_path) + 1);
    if (file_path == NULL) {
        log_error("Failed to allocate memory for file_path\n");
        return -1;
    }

    file_path[0] = '\0';

    /**
     * Remove the leading slash if the path is not the root path.
     */
    if (strcmp(path, "/") != 0) {
        path++;
    }

    if (build_absolute_path(file_path, path) == -1) {
        return -1;
    }

    long file_size = calculate_file_size(file_path);
    if (file_size == -1) {
        free(file_path);
        file_path = NULL;
        return -1;
    }

    char *response;
    size_t response_length = ((size_t)(file_size)) + response_headers_length;
    response = (char *)malloc(response_length * (sizeof *response) + 1);
    if (response == NULL) {
        log_error("Failed to allocate memory for response\n");
        free(file_path);
        file_path = NULL;
        return -1;
    }

    response[0] = '\0';

    /* Add the response headers to buffer */
    strcpy(response, response_headers);

    /* Add the file contents to buffer */
    if (read_file(response + response_headers_length, file_path, file_size) == -1) {
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

int construct_public_route_file_path(char **path_buffer, char *url) {
    char public_folder[] = "/src/web/pages/public";
    char file_extension[] = ".html";

    *path_buffer = (char *)malloc((strlen(public_folder) + strlen(url) + strlen(file_extension)) * (sizeof **path_buffer) + 1);
    if (*path_buffer == NULL) {
        log_error("Failed to allocate memory for *path_buffer\n");
        return -1;
    }

    sprintf(*path_buffer, "%s%s%s", public_folder, url, file_extension);

    return 0;
}

unsigned int requested_public_route(char *url) {
    char *public_routes[] = {"/home", "/login", "/sign-up", "/about", NULL};

    int i;
    for (i = 0; public_routes[i] != NULL; i++) {
        if (strcmp(url, public_routes[i]) == 0) {
            return 0;
        }
    }

    return 1;
}