#include "utils/utils.h"
#include <ctype.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "web/web.h"

/* Reviewed: Fri 17. Feb 2024 */
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

    ssize_t file_size = calculate_file_size(absolute_path);
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

    if (memcpy(*response_buffer, response_headers, response_headers_length) == NULL) {
        log_error("Failed to copy response headers into response\n");
        free(absolute_path);
        absolute_path = NULL;
        free(*response_buffer);
        *response_buffer = NULL;
        return -1;
    }

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

/* Reviewed: Fri 18. Feb 2024 */
char ***web_utils_matrix_2d_allocation(char ***p_matrix, unsigned int level1_size, unsigned int level2_size) {
    p_matrix = (char ***)malloc(level1_size * sizeof(char **));
    if (p_matrix == NULL) {
        log_error("Failed to allocate memory for p_matrix level 1\n");
        return NULL;
    }

    unsigned int i;
    for (i = 0; i < level1_size; ++i) {
        p_matrix[i] = (char **)malloc(level2_size * sizeof(char *));
        if (p_matrix[i] == NULL) {
            log_error("Failed to allocate memory for p_matrix level 2\n");
            unsigned int k;
            for (k = 0; k < i; ++k) {
                free(p_matrix[k]);
                p_matrix[k] = NULL;
            }
            free(p_matrix);
            p_matrix = NULL;
            return NULL;
        }
    }

    return p_matrix;
}

/* Reviewed: Fri 18. Feb 2024 */
void web_utils_matrix_2d_free(char ***p_matrix, unsigned int level1_size) {
    unsigned int i;

    for (i = 0; i < level1_size; i++) {
        free(p_matrix[i]);
        p_matrix[i] = NULL;
    }

    free(p_matrix);
    p_matrix = NULL;
}

/* Reviewed: Fri 17. Feb 2024 */
int web_utils_parse_http_request(HttpRequest *parsed_http_request, const char *http_request) {
    parsed_http_request->method = NULL;
    parsed_http_request->url = NULL;
    parsed_http_request->query_params = NULL;
    parsed_http_request->body = NULL;
    parsed_http_request->http_version = NULL;
    parsed_http_request->headers = NULL;

    /**
     * Extract http request method
     */
    const char *method_end = strchr(http_request, ' ');
    size_t method_length = method_end - http_request;
    parsed_http_request->method = (char *)malloc(method_length * (sizeof parsed_http_request->method) + 1);
    if (parsed_http_request->method == NULL) {
        log_error("Failed to allocate memory for parsed_http_request->method\n");
        return -1;
    }

    if (memcpy(parsed_http_request->method, http_request, method_length) == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to copy method from http request into structure\n");
        return -1;
    }

    parsed_http_request->method[method_length] = '\0';

    /**
     * Extract http request url and url query params
     */
    const char *url_start = method_end + 1;
    const char *url_end;
    if ((url_end = strchr(url_start, ' ')) == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to find char\n");
        return -1;
    }

    const char *query_params_start = url_start;

    int query_params_start_index = -1; /* we start assuming there isn't any query params */
    /* iterate though a string until a whitespace if found */
    while (!isspace((unsigned char)*query_params_start)) {
        /* if a '?' char is found along the way, that is the start of the query params */
        if (*query_params_start == '?') {
            query_params_start_index = query_params_start - http_request;
            break;
        }

        /* keep moving forward along the string */
        query_params_start++;
    }

    size_t query_params_length = 0;
    if (query_params_start_index >= 0) {
        query_params_length = url_end - (query_params_start + 1); /* skip '?' char at the beginning of query params */
        /**
         * because we separate the 'http request url' and 'http request url query params', we set the end of the 'http request url'
         * to where the 'http request url query params' start.
         */
        url_end = &http_request[query_params_start_index];
    }

    size_t url_len = url_end - url_start;

    parsed_http_request->url = (char *)malloc(url_len * (sizeof parsed_http_request->url) + 1);
    if (parsed_http_request->url == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to allocate memory for parsed_http_request->url\n");
        return -1;
    }

    if (memcpy(parsed_http_request->url, url_start, url_len) == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to copy url from http request into structure\n");
        return -1;
    }

    parsed_http_request->url[url_len] = '\0';

    if (query_params_length > 0) {
        parsed_http_request->query_params = (char *)malloc(query_params_length * (sizeof parsed_http_request->query_params) + 1);
        if (parsed_http_request->query_params == NULL) {
            web_utils_http_request_free(parsed_http_request);
            log_error("Failed to allocate memory for parsed_http_request->query_params\n");
            return -1;
        }

        /* (query_params_start + 1) -> skip '?' char at the beginning of query params */
        if (memcpy(parsed_http_request->query_params, (query_params_start + 1), query_params_length) == NULL) {
            web_utils_http_request_free(parsed_http_request);
            log_error("Failed to copy url query params from http request into structure\n");
            return -1;
        }

        parsed_http_request->query_params[query_params_length] = '\0';
    }

    /**
     * Extract http version from request
     */
    char *http_version_start;
    if ((http_version_start = strchr(query_params_start, ' ')) == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to find char\n");
        return -1;
    }

    http_version_start++; /* skip space */

    char *http_version_end;
    if ((http_version_end = strstr(http_version_start, "\r\n")) == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to find string\n");
        return -1;
    }

    size_t http_version_len = http_version_end - http_version_start;
    parsed_http_request->http_version = (char *)malloc(http_version_len * (sizeof parsed_http_request->http_version) + 1);
    if (parsed_http_request->http_version == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to allocate memory for parsed_http_request->http_version\n");
        return -1;
    }

    if (memcpy(parsed_http_request->http_version, http_version_start, http_version_len) == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to copy http version from http request into structure\n");
        return -1;
    }

    parsed_http_request->http_version[http_version_len] = '\0';

    /**
     * Extract http request headers
     */
    char *headers_start = http_version_end + 2; /* skip "\r\n" */
    char *headers_end;
    if ((headers_end = strstr(headers_start, "\r\n\r\n")) == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to find string\n");
        return -1;
    }

    size_t headers_len = headers_end - headers_start;
    parsed_http_request->headers = (char *)malloc(headers_len * (sizeof parsed_http_request->headers) + 1);
    if (parsed_http_request->headers == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to allocate memory for parsed_http_request->headers\n");
        return -1;
    }

    if (memcpy(parsed_http_request->headers, headers_start, headers_len) == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to copy headers from http request into structure\n");
        return -1;
    }

    parsed_http_request->headers[headers_len] = '\0';

    /**
     * Extract http request body
     */
    char *body_start = headers_end + 4; /* skip "\r\n\r\n" */
    char *body_end;
    if ((body_end = strchr(body_start, '\0')) == NULL) {
        web_utils_http_request_free(parsed_http_request);
        log_error("Failed to find char\n");
        return -1;
    }

    size_t body_length = body_end - body_start;
    if (body_length > 0) {
        parsed_http_request->body = (char *)malloc(body_length * (sizeof parsed_http_request->body) + 1);
        if (parsed_http_request->body == NULL) {
            web_utils_http_request_free(parsed_http_request);
            log_error("Failed to allocate memory for parsed_http_request->body\n");
            return -1;
        }

        if (memcpy(parsed_http_request->body, body_start, body_length) == NULL) {
            web_utils_http_request_free(parsed_http_request);
            log_error("Failed to copy request body from http request into structure\n");
            return -1;
        }

        parsed_http_request->body[body_length] = '\0';
    }

    return 0;
}

/* Reviewed: Fri 17. Feb 2024 */
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

char web_utils_hex_to_char(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    return -1;
}

int web_utils_url_decode(char **string) {
    char *out = *string;
    size_t len = strlen(*string);

    size_t i;
    for (i = 0; i < len; i++) {
        if ((*string)[i] == '%' && i + 2 < len && isxdigit((*string)[i + 1]) && isxdigit((*string)[i + 2])) {
            char c = web_utils_hex_to_char((*string)[i + 1]) * 16 + web_utils_hex_to_char((*string)[i + 2]);
            *out++ = c;
            i += 2;
        } else if ((*string)[i] == '+') {
            *out++ = ' ';
        } else {
            *out++ = (*string)[i];
        }
    }

    *out = '\0';

    return 0;
}
