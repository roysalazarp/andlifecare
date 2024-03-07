#ifndef WEB_H
#define WEB_H

typedef struct {
    char *method;
    char *url;
    char *query_params;
    char *http_version;
    char *headers;
    char *body;
} HttpRequest;

char ***web_utils_matrix_2d_allocation(char ***p_matrix, unsigned short level1_size, unsigned short level2_size);
void web_utils_matrix_2d_free(char ***p_matrix, unsigned short level1_size);
int web_utils_parse_http_request(HttpRequest *parsed_http_request, const char *http_request);
void web_utils_http_request_free(HttpRequest *parsed_http_request);
int web_utils_parse_value(char **buffer, const char key_name[], char *string);
int web_utils_url_decode(char **string);

int web_static(int client_socket, char *path, const char *response_headers, size_t response_headers_length);
int construct_public_route_file_path(char **path_buffer, char *url);
unsigned int requested_public_route(const char *url);

int web_ui_test_get(int client_socket, HttpRequest *request, int conn_index);

int web_home_get(int client_socket, HttpRequest *request);

int web_sign_up_get(int client_socket, HttpRequest *request);
int web_sign_up_create_user_post(int client_socket, HttpRequest *request, int conn_index);

int web_not_found(int client_socket, HttpRequest *request);

#endif