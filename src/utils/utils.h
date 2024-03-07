#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <unistd.h>

ssize_t calculate_file_size(char *file_path);
int read_file(char *file_content, char *file_path, size_t file_size);
int read_file_from_path_relative_to_project_root(char **buffer, const char *file_path_relative_to_project_root);
int build_absolute_path(char *buffer, const char *path_relative_to_project_root);
int load_values_from_file(void *structure, const char *file_path_relative_to_project_root);

#endif