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

int home_get(int client_socket, char *request) {
    char *template_path;
    template_path = (char*)malloc(PATH_MAX * (sizeof *template_path) + 1);
    if (template_path == NULL) {
        log_error("Failed to allocate memory for template_path\n");
        return -1;
    }
    
    template_path[0] = '\0';

    char path[] = "src/web/pages/home/home.html";

    if (sprintf(template_path, "%s%s", CWD, path) < 0) {
        log_error("Formatted string truncated\n");
        free(template_path);
        template_path = NULL;
        return -1;
    }

    long template_length = calculate_file_bytes_length(template_path);
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
    size_t original_response_length = ((size_t)(template_length)) + response_headers_length;
    response = (char*)malloc(original_response_length * (sizeof *response) + 1);

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

    response[original_response_length] = '\0';

    char *country[] = { "v0", "Finland" };
    if (te_single_substring_swap(country[0], country[1], &response) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

    char *phone_number[] = { "v1", "441317957" };
    if (te_single_substring_swap(phone_number[0], phone_number[1], &response) == -1) {
        free(response);
        response = NULL;
        return -1;
    }
    
    char *phone_prefix[] = { "v2", "+358" };
    if (te_single_substring_swap(phone_prefix[0], phone_prefix[1], &response) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

    char *menu_list[] = { "for0", "for0->v0", "home", "about", "contact", "careers", NULL };
    if (te_multiple_substring_swap(menu_list[0], menu_list[1], menu_list + 2, &response) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

    if (send(client_socket, response, strlen(response), 0) == -1) {
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
