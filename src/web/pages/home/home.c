#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/limits.h>
#include <stdarg.h>

#include "utils/utils.h"
#include "template_engine/template_engine.h"
#include "globals.h"
#include "core/core.h"

void free_users_list(int num_rows, int num_columns, char ***users_list);

int home_get(int client_socket, char *request) {
    char *template_path;
    template_path = (char*)malloc(PATH_MAX * (sizeof *template_path) + 1);
    if (template_path == NULL) {
        log_error("Failed to allocate memory for template_path\n");
        return -1;
    }
    
    template_path[0] = '\0';

    if (build_absolute_path(template_path, "/src/web/pages/home/home.html") == -1) {
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

    char response_headers[] =   "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/html\r\n"
                                "\r\n";

    size_t response_headers_length = strlen(response_headers) * sizeof(char);

    char *response;
    size_t pre_rendering_response_length = ((size_t)(template_length)) + response_headers_length;
    response = (char*)malloc(pre_rendering_response_length * (sizeof *response) + 1);

    if (response == NULL) {
        log_error("Failed to allocate memory for response\n");
        free(template_path);
        template_path = NULL;
        return -1;
    }

    response[0] = '\0';
    
    strcpy(response, response_headers);

    if (read_file(response + response_headers_length, template_path, template_length) == -1) {
        free(template_path);
        template_path = NULL;
        free(response);
        response = NULL;
        return -1;
    }

    free(template_path);
    template_path = NULL;

    response[pre_rendering_response_length] = '\0';

    User *users;
    int num_rows;
    int num_columns;

    if (core_view_home(&users, &num_rows, &num_columns) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

    char ***users_list;
    users_list = (char ***)malloc(num_rows * sizeof(char **));
    if (users_list == NULL) {
        log_error("Failed to allocate memory for users_list\n");
        free(response);
        response = NULL;
        free(users);
        users = NULL;
        return -1;
    }
    
    int i;
    for (i = 0; i < num_rows; ++i) {
        users_list[i] = (char **)malloc((num_columns + 1) * sizeof(char *));

        if (users_list[i] == NULL) {
            log_error("Failed to allocate memory for users_list[i]\n");
            free_users_list(i, num_columns, users_list);
            free(response);
            response = NULL;
            free(users);
            users = NULL;
            return -1;
        }
        
        users_list[i][0] = (char *)malloc((strlen(users[i].id) + 1) * sizeof(char));
        users_list[i][1] = (char *)malloc((strlen(users[i].email) + 1) * sizeof(char));
        users_list[i][2] = (char *)malloc((strlen(users[i].country) + 1) * sizeof(char));
        users_list[i][3] = (char *)malloc((strlen(users[i].full_name) + 1) * sizeof(char));
        
        /**
         * NOTE: This check for memory allocation failure probably doesn't work because 
         *       free_users_list has no way to know which columns were allocated and which weren't.
         */
        if (users_list[i][0] == NULL || users_list[i][1] == NULL || users_list[i][2] == NULL || users_list[i][3] == NULL) {
            log_error("Failed to allocate memory for users_list[i][j]\n");
            free_users_list(i, num_columns, users_list);
            free(response);
            response = NULL;
            free(users);
            users = NULL;
            return -1;
        }

        strcpy(users_list[i][0], users[i].id);
        strcpy(users_list[i][1], users[i].email);
        strcpy(users_list[i][2], users[i].country);
        strcpy(users_list[i][3], users[i].full_name);        
        users_list[i][4] = NULL;
    }

    free(users);
    users = NULL;

    if (te_multiple_substring_swap("for0", (size_t)num_columns, users_list, &response, (size_t)num_rows) == -1) {
        free_users_list(num_rows, num_columns, users_list);
        free(response);
        response = NULL;
        return -1;
    }

    free_users_list(num_rows, num_columns, users_list);

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