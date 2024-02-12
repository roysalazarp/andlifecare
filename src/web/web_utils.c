#include "utils/utils.h"
#include <ctype.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "web/web.h"

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

    memcpy(*response_buffer, response_headers, response_headers_length);

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

int web_utils_parse_http_request(HttpRequest *parsed_http_request, const char *http_request) {
    parsed_http_request->method = NULL;
    parsed_http_request->url = NULL;
    parsed_http_request->query_params = NULL;
    parsed_http_request->body = NULL;
    parsed_http_request->http_version = NULL;
    parsed_http_request->headers = NULL;

    const char *method_end = strchr(http_request, ' ');
    size_t method_len = method_end - http_request;
    parsed_http_request->method = (char *)malloc(method_len + 1);
    if (parsed_http_request->method == NULL) {
        log_error("Failed to allocate memory for parsed_http_request->method\n");
        return -1;
    }
    memcpy(parsed_http_request->method, http_request, method_len);
    parsed_http_request->method[method_len] = '\0';

    const char *url_start = method_end + 1;
    const char *url_end = strchr(url_start, ' ');

    const char *question_mark = url_start; /* we start assuming there isn't any query params */

    int query_params_start = -1;
    while (!isspace((unsigned char)*question_mark)) {
        if (*question_mark == '?') {
            query_params_start = question_mark - http_request;
            break;
        }

        question_mark++;
    }

    size_t query_params_len = 0;
    if (query_params_start >= 0) {

        query_params_len = url_end - (question_mark + 1); /* skip question mark itself */
        url_end = &http_request[query_params_start];
    }

    size_t url_len = url_end - url_start;

    parsed_http_request->url = (char *)malloc(url_len + 1);
    if (parsed_http_request->url == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to allocate memory for parsed_http_request->url\n");
        return -1;
    }
    memcpy(parsed_http_request->url, url_start, url_len);
    parsed_http_request->url[url_len] = '\0';

    if (query_params_len > 0) {
        parsed_http_request->query_params = (char *)malloc(query_params_len + 1);
        if (parsed_http_request->query_params == NULL) {
            web_utils_http_request_free(parsed_http_request);
            log_error("Failed to allocate memory for parsed_http_request->query_params\n");
            return -1;
        }
        memcpy(parsed_http_request->query_params, question_mark + 1, query_params_len); /* skip question mark itself */
        parsed_http_request->query_params[query_params_len] = '\0';
    }

    char *http_version_start = strchr(question_mark, ' ');
    http_version_start++; /* skip space itself */
    char *http_version_end = strstr(http_version_start, "\r\n");
    size_t http_version_len = http_version_end - http_version_start;
    parsed_http_request->http_version = (char *)malloc(http_version_len + 1);
    if (parsed_http_request->http_version == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to allocate memory for parsed_http_request->http_version\n");
        return -1;
    }
    memcpy(parsed_http_request->http_version, http_version_start, http_version_len);
    parsed_http_request->http_version[http_version_len] = '\0';

    char *headers_start = http_version_end + 2; /* skip "\r\n" */
    char *headers_end = strstr(headers_start, "\r\n\r\n");
    size_t headers_len = headers_end - headers_start;
    parsed_http_request->headers = (char *)malloc(headers_len + 1);
    if (parsed_http_request->headers == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to allocate memory for parsed_http_request->headers\n");
        return -1;
    }
    memcpy(parsed_http_request->headers, headers_start, headers_len);
    parsed_http_request->headers[headers_len] = '\0';

    char *body_start = headers_end + 4; /* skip "\r\n\r\n" */
    char *body_end = strchr(body_start, '\0');
    size_t body_len = body_end - body_start;
    if (body_len > 0) {
        parsed_http_request->body = (char *)malloc(body_len + 1);
        if (parsed_http_request->body == NULL) {
            web_utils_http_request_free(parsed_http_request);
            log_error("Failed to allocate memory for parsed_http_request->body\n");
            return -1;
        }
        memcpy(parsed_http_request->body, body_start, body_len);
        parsed_http_request->body[body_len] = '\0';
    }

    return 0;
}

void web_utils_http_request_free(HttpRequest *parsed_http_request) {
    free(parsed_http_request->method);
    parsed_http_request->method = NULL;

    free(parsed_http_request->url);
    parsed_http_request->url = NULL;

    free(parsed_http_request->query_params);
    parsed_http_request->query_params = NULL;

    free(parsed_http_request->body);
    parsed_http_request->body = NULL;

    free(parsed_http_request->http_version);
    parsed_http_request->http_version = NULL;

    free(parsed_http_request->headers);
    parsed_http_request->headers = NULL;
}

int web_utils_parse_value(char **buffer, const char key_name[], char *string) {
    char *key = string;

    while (*key != '\0') {
        char *key_end = strchr(key, '=');
        size_t key_length = key_end - key;

        char *value_start = key_end + 1;
        char *value_end;
        value_end = strchr(value_start, '&');
        if (value_end == NULL) {
            value_end = strchr(value_start, '\0');
        }

        size_t value_length = value_end - value_start;

        if (strncmp(key, key_name, key_length) == 0) {
            *buffer = malloc(value_length + 1);
            if (*buffer == NULL) {
                log_error("Failed to allocate memory for *buffer\n");
                return -1;
            }

            memcpy(*buffer, value_start, value_length);
            (*buffer)[value_length] = '\0';
        }

        if (*value_end == '\0') {
            break;
        }

        key = value_end + 1;
    }

    if (*buffer == NULL) {
        log_error("Value not found per key\n");
        return -1;
    }

    return 0;
}