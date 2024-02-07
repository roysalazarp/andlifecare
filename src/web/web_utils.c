#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/utils.h"

int web_utils_construct_response(char **response_buffer, const char *file_path, const char *response_headers) {
    char *absolute_path;
    absolute_path = (char *)malloc(PATH_MAX * (sizeof *absolute_path) + 1);
    if (absolute_path == NULL) {
        log_error("Failed to allocate memory for absolute_path\n");
        return -1;
    }

    absolute_path[0] = '\0';

    if (build_absolute_path(absolute_path, file_path) == -1) {
        free(absolute_path);
        absolute_path = NULL;
        return -1;
    }

    long file_size = calculate_file_size(absolute_path);
    if (file_size == -1) {
        free(absolute_path);
        absolute_path = NULL;
        return -1;
    }

    size_t response_headers_length = strlen(response_headers) * sizeof(char);
    size_t pre_rendering_response_length = (size_t)file_size + response_headers_length;
    *response_buffer = (char *)malloc(pre_rendering_response_length * (sizeof **response_buffer) + 1);
    if (*response_buffer == NULL) {
        log_error("Failed to allocate memory for *response_buffer\n");
        free(absolute_path);
        absolute_path = NULL;
        return -1;
    }

    strncpy(*response_buffer, response_headers, response_headers_length);

    if (read_file(*response_buffer + response_headers_length, absolute_path, file_size) == -1) {
        free(absolute_path);
        absolute_path = NULL;
        free(*response_buffer);
        *response_buffer = NULL;
        return -1;
    }

    free(absolute_path);
    absolute_path = NULL;

    (*response_buffer)[pre_rendering_response_length] = '\0';

    return 0;
}

char ***web_utils_matrix_2d_allocation(char ***p, int d1, int d2) {
    int i;
    p = (char ***)malloc(d1 * sizeof(char **));
    if (p == NULL) {
        log_error("Failed to allocate memory for p\n");
        return NULL;
    }

    for (i = 0; i < d1; ++i) {
        p[i] = (char **)malloc(d2 * sizeof(char *));
        if (p[i] == NULL) {
            log_error("Failed to allocate memory for p[i]\n");
            int j;
            for (j = 0; j < i; ++j) {
                free(p[j]);
                p[j] = NULL;
            }
            free(p);
            p = NULL;
            return NULL;
        }
    }

    return p;
}

void web_utils_matrix_2d_free(char ***p, int d1, int d2) {
    int i;

    for (i = 0; i < d1; i++) {
        free(p[i]);
        p[i] = NULL;
    }

    free(p);
    p = NULL;
}