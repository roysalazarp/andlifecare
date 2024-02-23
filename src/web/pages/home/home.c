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
#include "web/web.h"

int web_page_home_get(int client_socket, HttpRequest *request) {
    /** TODO: improve http response headers */
    char response_headers[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "\r\n";

    char *response;
    if (web_utils_construct_response(&response, "src/web/pages/home/home.html", response_headers) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

    if (te_single_substring_swap("{{ hello_world }}", "hello world", &response) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

    size_t post_rendering_response_length = strlen(response) * sizeof(char);

    if (send(client_socket, response, post_rendering_response_length, 0) == -1) {
        fprintf(stderr, "Failed send HTTP response\nError code: %d\n", errno);
        free(response);
        response = NULL;
        return -1;
    }

    free(response);
    response = NULL;

    return 0;
}
