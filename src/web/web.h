#ifndef WEB_H
#define WEB_H

int web_utils_construct_response(char **response, const char *file_path, const char *response_headers);
char ***web_utils_matrix_2d_allocation(char ***p, int d1, int d2);
void web_utils_matrix_2d_free(char ***p, int d1, int d2);

int web_serve_static(int client_socket, char *path, const char *response_headers, size_t response_headers_length);

int web_page_ui_test_get(int client_socket, char *request);

int web_page_home_get(int client_socket, char *request);

int web_page_not_found(int client_socket, char *request);

#endif