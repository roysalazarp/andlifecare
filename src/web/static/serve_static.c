#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "globals.h"
#include "utils/utils.h"

int web_static(int client_socket, char *path, const char *response_headers, size_t response_headers_length) {
    char *file_absolute_path;
    file_absolute_path = (char *)malloc(PATH_MAX * (sizeof *file_absolute_path) + 1);
    if (file_absolute_path == NULL) {
        fprintf(stderr, "Failed to allocate memory for file_absolute_path\nError code: %d\n", errno);
        return -1;
    }

    file_absolute_path[0] = '\0';

    /** Remove the leading slash if path is NOT the root url (home page) */
    if (strcmp(path, "/") != 0) {
        path++;
    }

    if (build_absolute_path(file_absolute_path, path) == -1) {
        return -1;
    }

    ssize_t file_size = calculate_file_size(file_absolute_path);
    if (file_size == -1) {
        free(file_absolute_path);
        file_absolute_path = NULL;
        return -1;
    }

    char *response;
    size_t response_length = ((size_t)(file_size)) + response_headers_length;
    response = (char *)malloc(response_length * (sizeof *response) + 1);
    if (response == NULL) {
        fprintf(stderr, "Failed to allocate memory for response\nError code: %d\n", errno);
        free(file_absolute_path);
        file_absolute_path = NULL;
        return -1;
    }

    response[0] = '\0';

    /* Add headers to response */
    strcpy(response, response_headers);

    /* Add the file contents to the response */
    if (read_file(response + response_headers_length, file_absolute_path, file_size) == -1) {
        free(file_absolute_path);
        file_absolute_path = NULL;
        free(response);
        response = NULL;
        return -1;
    }

    response[response_length] = '\0';

    free(file_absolute_path);
    file_absolute_path = NULL;

    if (send(client_socket, response, response_length, 0) == -1) {
        fprintf(stderr, "Failed send HTTP response\nError code: %d\n", errno);
        free(response);
        response = NULL;
        return -1;
    }

    close(client_socket);

    free(response);
    response = NULL;

    return 0;
}

int construct_public_route_file_path(char **path_buffer, char *url) {
    char public_folder[] = "/src/web/pages/public";
    char file_extension[] = ".html";

    *path_buffer = (char *)malloc((strlen(public_folder) + strlen(url) + strlen(file_extension)) * (sizeof **path_buffer) + 1);
    if (*path_buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for *path_buffer\nError code: %d\n", errno);
        return -1;
    }

    if (sprintf(*path_buffer, "%s%s%s", public_folder, url, file_extension) < 0) {
        fprintf(stderr, "Failed to construct public route file path\nError code: %d\n", errno);
        free(*path_buffer);
        *path_buffer = NULL;
        return -1;
    }

    return 0;
}

unsigned int requested_public_route(const char *url) {
    char *public_routes[] = {"/home", "/about", NULL};

    unsigned short i;
    for (i = 0; public_routes[i] != NULL; i++) {
        if (strcmp(url, public_routes[i]) == 0) {
            return 0;
        }
    }

    return 1;
}