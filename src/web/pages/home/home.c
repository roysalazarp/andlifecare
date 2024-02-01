#include <linux/limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "core/core.h"
#include "globals.h"
#include "template_engine/template_engine.h"
#include "utils/utils.h"

void free_users_list(int num_rows, int num_columns, char ***users_list);
int construct_response(char **response, const char *file_path, const char *response_headers);
int te_multiply_substring(char **string, const char *start_token, const char *end_token, int times);

int home_get(int client_socket, char *request) {
    int i;
    char response_headers[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "\r\n";
    char *response;
    if (construct_response(&response, "src/web/pages/home/home.html", response_headers) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

    User *users;
    int num_rows;
    int num_columns;

    if (core_view_home(&users, &num_rows, &num_columns) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

    char ***column_names;
    column_names = (char ***)malloc(4 * sizeof(char **));
    column_names[0] = (char **)malloc(1 * sizeof(char *));
    column_names[1] = (char **)malloc(1 * sizeof(char *));
    column_names[2] = (char **)malloc(1 * sizeof(char *));
    column_names[3] = (char **)malloc(1 * sizeof(char *));
    column_names[0][0] = "Id";
    column_names[1][0] = "Name";
    column_names[2][0] = "Email";
    column_names[3][0] = "Country";

    if (te_multiple_substring_swap("{{ for->column_names }}", "{{ end for->column_names }}", 1, column_names, &response, 4) == -1) {
        free_users_list(4, 1, column_names);
        free(response);
        response = NULL;
        return -1;
    }

    if (te_multiple_substring_swap("{{ for->rows }}", "{{ end for->rows }}", 0, NULL, &response, num_rows) == -1) {
        /*
        free_users_list(num_rows, num_columns, users_list);
        */
        free(response);
        response = NULL;
        return -1;
    }

    char ****users_list;
    users_list = (char ****)malloc(num_rows * sizeof(char ***));
    if (users_list == NULL) {
        log_error("Failed to allocate memory for users_list\n");
        free(response);
        response = NULL;
        free(users);
        users = NULL;
        return -1;
    }

    for (i = 0; i < num_rows; ++i) {
        users_list[i] = (char ***)malloc((num_columns + 1) * sizeof(char **));

        if (users_list[i] == NULL) {
            log_error("Failed to allocate memory for users_list[i]\n");
            /*
            free_users_list(i, num_columns, users_list);
            */
            free(response);
            response = NULL;
            free(users);
            users = NULL;
            return -1;
        }

        users_list[i][0] = (char **)malloc(1 * sizeof(char *));
        users_list[i][0][0] = (char *)malloc((strlen(users[i].id) + 1) * sizeof(char));

        users_list[i][1] = (char **)malloc(1 * sizeof(char *));
        users_list[i][1][0] = (char *)malloc((strlen(users[i].full_name) + 1) * sizeof(char));

        users_list[i][2] = (char **)malloc(1 * sizeof(char *));
        users_list[i][2][0] = (char *)malloc((strlen(users[i].email) + 1) * sizeof(char));

        users_list[i][3] = (char **)malloc(1 * sizeof(char *));
        users_list[i][3][0] = (char *)malloc((strlen(users[i].country) + 1) * sizeof(char));

        /**
         * NOTE: This check for memory allocation failure probably
         * doesn't work because free_users_list has no way to know which
         * columns were allocated and which weren't.
         */
        if (users_list[i][0] == NULL || users_list[i][1] == NULL || users_list[i][2] == NULL || users_list[i][3] == NULL) {
            log_error("Failed to allocate memory for users_list[i][j]\n");
            /*
            free_users_list(i, num_columns, users_list);
            */
            free(response);
            response = NULL;
            free(users);
            users = NULL;
            return -1;
        }

        strcpy(users_list[i][0][0], users[i].id);
        strcpy(users_list[i][1][0], users[i].full_name);
        strcpy(users_list[i][2][0], users[i].email);
        strcpy(users_list[i][3][0], users[i].country);

        if (te_multiple_substring_swap("{{ for->row_values }}", "{{ end for->row_values }}", 1, (char ***)users_list[i], &response, 4) == -1) {
            /*
            free_users_list(num_rows, num_columns, users_list);
            */
            free(response);
            response = NULL;
            return -1;
        }
    }

    free(users);
    users = NULL;

    /*
    if (te_multiple_substring_swap("{{ for->row_values }}", "{{ end for->row_values }}", (size_t)num_columns, users_list, &response, (size_t)num_rows) == -1) {
        free_users_list(num_rows, num_columns, users_list);
        free(response);
        response = NULL;
        return -1;
    }
    */

    /*
     free_users_list(num_rows, num_columns, users_list);
    */

    size_t post_rendering_response_length = strlen(response) * sizeof(char);

    if (send(client_socket, response, post_rendering_response_length, 0) == -1) {
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

void home_post(int client_socket, char *request) {}
void home_put(int client_socket, char *request) {}
void home_patch(int client_socket, char *request) {}

void free_users_list(int num_rows, int num_columns, char ***users_list) {
    int i;
    int j;
    for (i = 0; i < num_rows; ++i) {
        for (j = 0; j < num_columns; ++j) {
            free(users_list[i][j]);
            users_list[i][j] = NULL;
        }
        free(users_list[i]);
        users_list[i] = NULL;
    }

    free(users_list);
}

int construct_response(char **response, const char *file_path, const char *response_headers) {
    char *template_path;
    template_path = (char *)malloc(PATH_MAX * (sizeof *template_path) + 1);
    if (template_path == NULL) {
        log_error("Failed to allocate memory for template_path\n");
        return -1;
    }

    template_path[0] = '\0';

    if (build_absolute_path(template_path, file_path) == -1) {
        free(template_path);
        template_path = NULL;
        return -1;
    }

    long template_length = calculate_file_size(template_path);
    if (template_length == -1) {
        free(template_path);
        template_path = NULL;
        return -1;
    }

    size_t response_headers_length = strlen(response_headers) * sizeof(char);

    size_t pre_rendering_response_length = ((size_t)(template_length)) + response_headers_length;
    *response = (char *)malloc(pre_rendering_response_length * (sizeof **response) + 1);

    if (*response == NULL) {
        log_error("Failed to allocate memory for *response\n");
        free(template_path);
        template_path = NULL;
        return -1;
    }

    *response[0] = '\0';

    strcpy(*response, response_headers);

    if (read_file(*response + response_headers_length, template_path, template_length) == -1) {
        free(template_path);
        template_path = NULL;
        free(*response);
        *response = NULL;
        return -1;
    }

    free(template_path);
    template_path = NULL;

    /*
     * IMPORTANT: Check why this is failing
     * response[pre_rendering_response_length] = '\0';
     */
    return 0;
}

int te_multiply_substring(char **string, const char *start_token, const char *end_token, int times) {
    char *start_address = te_substring_address_find(start_token, *string, 0, 1);
    unsigned int block_start_position = start_address - *string;
    char *end_address = te_substring_address_find(end_token, *string, block_start_position, 1);

    int block_length = end_address - (start_address + 16);

    char *for_template;
    for_template = (char *)malloc((block_length * (sizeof *for_template)) * times + 1);
    if (for_template == NULL) {
        log_error("Failed to allocate memory for for_template\n");
        return -1;
    }

    for_template[0] = '\0';

    int i;
    for (i = 0; i < times; ++i) {
        unsigned int at = i * block_length;

        if (strncpy(for_template + at, start_address + strlen(start_token), block_length) == NULL) {
            log_error("Failed copy string\n");
            free(for_template);
            for_template = NULL;
            return -1;
        }
    }

    te_substring_copy_into_string_at_memory_space(for_template, string, start_address, end_address + strlen(end_token));
    return 0;
}