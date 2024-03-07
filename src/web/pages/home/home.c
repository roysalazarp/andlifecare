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

int web_home_get(int client_socket, HttpRequest *request) {
    /** TODO: improve http response headers */
    char response_headers[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "\r\n";

    char *template;
    if (read_file_from_path_relative_to_project_root(&template, "src/web/pages/home/home.html") == -1) {
        free(template);
        template = NULL;
        return -1;
    }

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

    close(client_socket);

    free(response);
    response = NULL;

    return 0;
}
