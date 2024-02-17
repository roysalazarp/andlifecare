#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <unistd.h>

void log_error(const char *message);
ssize_t calculate_file_size(char *file_path);
int read_file(char *file_content, char *file_path, size_t file_size);
int build_absolute_path(char *buffer, const char *path);
int load_values_from_file(void *structure, const char *path);

#endif