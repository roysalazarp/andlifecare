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

int web_page_sign_up_get(int client_socket, HttpRequest *request) {
    char response_headers[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "\r\n";

    char *response;
    if (web_utils_construct_response(&response, "src/web/pages/sign-up/sign-up.html", response_headers) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

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

int web_page_sign_up_create_user_post(int client_socket, HttpRequest *request) {
    char response_headers[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "\r\n";

    web_utils_url_decode(&request->body);

    char *email = NULL;
    web_utils_parse_value(&email, "email", request->body);

    char *password = NULL;
    web_utils_parse_value(&password, "password", request->body);

    char *repeat_password = NULL;
    web_utils_parse_value(&repeat_password, "repeat_password", request->body);

    char *remember = NULL;
    web_utils_parse_value(&remember, "remember", request->body);

    printf("%s\n", email);
    printf("%s\n", password);
    printf("%s\n", repeat_password);
    printf("%s\n", remember);

    free(email);
    email = NULL;

    free(password);
    password = NULL;

    free(repeat_password);
    repeat_password = NULL;

    free(remember);
    remember = NULL;

    if (send(client_socket, response_headers, strlen(response_headers), 0) == -1) {
        log_error("Failed send HTTP response\n");
        return -1;
    }

    close(client_socket);
    return 0;

    return 0;
}
