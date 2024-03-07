#include <argon2.h>
#include <errno.h>
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
#include "web/web.h"

int web_sign_up_get(int client_socket, HttpRequest *request) {
    /** TODO: improve http response headers */
    char response_headers[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "\r\n";

    char *loaded_file;
    if (read_file_from_path_relative_to_project_root(&loaded_file, "src/web/pages/sign_up/sign-up.html") == -1) {
        free(loaded_file);
        loaded_file = NULL;
        return -1;
    }

    char *template;
    if (te_copy_substring_block(&template, NULL, "{{ component->sign_up_page }}", "{{ end component->sign_up_page }}", &loaded_file) == -1) {
        free(loaded_file);
        loaded_file = NULL;
        free(template);
        template = NULL;
        return -1;
    }

    free(loaded_file);
    loaded_file = NULL;

    char *response;
    response = (char *)malloc((strlen(response_headers) + strlen(template)) * (sizeof *response) + 1);
    if (response == NULL) {
        fprintf(stderr, "Failed to allocate memory for response\nError code: %d\n", errno);
        return -1;
    }

    if (sprintf(response, "%s%s", response_headers, template) < 0) {
        fprintf(stderr, "Failed to construct response\nError code: %d\n", errno);
        free(template);
        template = NULL;
        free(response);
        response = NULL;
        return -1;
    }

    free(template);
    template = NULL;

    size_t post_rendering_response_length = strlen(response) * sizeof(char);

    if (send(client_socket, response, post_rendering_response_length, 0) == -1) {
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

int web_sign_up_create_user_post(int client_socket, HttpRequest *request, int conn_index) {
    /**
     * This endpoint may return:
     * - (Error) HTML partials to display error messages (invalid inputs, server errors...).
     * - (Success) Redirect instructions in the headers
     */

    char response_headers[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "\r\n";

    SignUpCreateUserInput input;
    web_utils_url_decode(&request->body);

    web_utils_parse_value(&input.email, "email", request->body);
    web_utils_parse_value(&input.password, "password", request->body);
    web_utils_parse_value(&input.repeat_password, "repeat_password", request->body);

    SignUpCreateUserResult result;
    /**
     * NOTE: Maybe core_sign_up_create_user should also accept a errors buffer that is an
     *       strings array. We can send html partials with error messages to the UI.
     */
    if (core_sign_up_create_user(&result, &input, client_socket, conn_index) == -1) {
        return -1;
    }

    if (send(client_socket, response_headers, strlen(response_headers), 0) == -1) {
        fprintf(stderr, "Failed send HTTP response\nError code: %d\n", errno);
        return -1;
    }

    close(client_socket);

    return 0;
}
