#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

void log_error(const char *message);
long int calculate_file_bytes_length(char *file_path);
int read_file(char *file_content, char *file_path, long file_size);
int build_absolute_path(char *buffer, const char *path);
size_t calculate_combined_strings_length(unsigned int num_strings, ...);

/** char *retrieve_header(const char *request, const char *key); */

#endif