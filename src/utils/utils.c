#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdarg.h>
#include <unistd.h>

void log_error(const char *message) {
    perror(message);
    fprintf(stderr, "Error code: %d\n", errno);
}

long calculate_file_bytes_length(char *absolute_file_path) {
    FILE* file = fopen(absolute_file_path, "r");

    if (file == NULL) {
        log_error("Error opening file\n");
        return -1;
    }

    if (fseek(file, 0, SEEK_END) == -1) {
        log_error("Failed to move the file position indicator to the end of the file\n");
        fclose(file);
        return -1;
    }

    long file_size = ftell(file);
    if (file_size == -1) {
        log_error("Failed to determine the current file position indicator of a file\n");
        fclose(file);
        return -1;
    }

    rewind(file);
    fclose(file);
    
    return file_size;
}

int read_file(char *file_content, char *absolute_file_path, long file_size) {
    FILE* file = fopen(absolute_file_path, "r");

    if (file == NULL) {
        log_error("Error opening file\n");
        return -1;
    }

    size_t bytes_read = fread(file_content, sizeof(char), (size_t)file_size, file);
    if (bytes_read != (size_t)file_size) {
        if (feof(file)) {
            log_error("End of file reached before reading all elements\n");
        } 

        if (ferror(file)) {
            perror("An error occurred during the fread operation");
        }

        fclose(file);
        return -1;
    }

    fclose(file);

    return 0;
}

int build_absolute_path(char *buffer, const char *path) {
    char *cwd;
    cwd = (char *)malloc(PATH_MAX * (sizeof *cwd));
    if (cwd == NULL) {
        log_error("Failed to allocate memory for cwd\n");
        free(cwd);
        cwd = NULL;
        return -1;
    }

    if (getcwd(cwd, PATH_MAX) == NULL) {
        log_error("Failed to get current working directory\n");
        free(cwd);
        cwd = NULL;
        return -1;
    }

    if (cwd == NULL) {
        log_error("Failed to get the absolute path of the current working directory\n");
        free(cwd);
        cwd = NULL;
        return -1;
    }

    /* snprintf automatically appends null-terminator */
    if (sprintf(buffer, "%s%s", cwd, path) < 0) {
        log_error("Formatted string truncated\n");
        free(cwd);
        cwd = NULL;
        return -1;
    }

    free(cwd);
    cwd = NULL;

    return 0;
}

size_t calculate_combined_strings_length(unsigned int num_strings, ...) {
    size_t error_value = 0;
    
    if (num_strings <= 0) {
        log_error("Invalid number of strings\n");
        return error_value;
    }
    
    size_t sum = 0;

    va_list args;
    va_start(args, num_strings);

    unsigned int i;
    for (i = 0; i < num_strings; ++i) {
        char *current_str = va_arg(args, char*);
        if (current_str == NULL) {
            va_end(args);
            log_error("string is NULL\n");
            return error_value;
        }
        sum += strlen(current_str);
    }

    va_end(args);

    return sum;
}

/*
char *retrieve_header(const char *request, const char *key) {
    char *key_start = strstr(request, key);
    if (key_start == NULL) return NULL;

    // Move the pointer to the start of the value and skip spaces and colon
    while (*key_start != '\0' && (*key_start == ' ' || *key_start == ':')) {
        key_start++;
    }

    char *value_end = strchr(key_start, '\n'); // Find the end of the value 
    if (value_end == NULL) return NULL;

    size_t value_length = value_end - key_start;
    
    char *value;
    value = (char*)malloc(value_length * (sizeof *value) + 1);
    if (value == NULL) {
        log_error("Failed to allocate memory\n");
        return NULL;
    }

    value[0] = '\0';

    strncpy(value, key_start, value_length);

    return value;
}
*/