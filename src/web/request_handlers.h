#ifndef REQUEST_HANDLERS_H
#define REQUEST_HANDLERS_H

int serve_static(int client_socket, char *path, const char *response_headers, size_t response_headers_length);
int home_get(int client_socket, char *request);
int not_found(int client_socket, char *request);

#endif