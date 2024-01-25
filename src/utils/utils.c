#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdarg.h>
#include <unistd.h>
#include <stddef.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 100

void log_error(const char *message) {
    perror(message);
    fprintf(stderr, "Error code: %d\n", errno);
}

long calculate_file_size(char *absolute_file_path) {
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

int read_file(char *buffer, char *absolute_file_path, long file_size) {
    FILE* file = fopen(absolute_file_path, "r");

    if (file == NULL) {
        log_error("Error opening file\n");
        return -1;
    }

    size_t bytes_read = fread(buffer, sizeof(char), (size_t)file_size, file);
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

    /* sprintf automatically appends null-terminator */
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

int file_content_to_string(char *buffer, size_t buffer_size, const char* path_from_project_root) {
    char *full_path;
    full_path = (char*)malloc(PATH_MAX * (sizeof *full_path) + 1);
    if (full_path == NULL) {
        log_error("Failed to allocate memory for full_path\n");
        return -1;
    }
    
    full_path[0] = '\0';

    if (build_absolute_path(full_path, path_from_project_root) == -1) {
        free(full_path);
        full_path = NULL;
        return -1;
    }

    if (read_file(buffer, full_path, buffer_size) == -1) {
        free(full_path);
        full_path = NULL;
        return -1;
    }

    buffer[buffer_size] = '\0';

    free(full_path);
    full_path = NULL;

    return 0;
}

/**
 * @brief      Given the file path of a file with KEY=value pairs and a set of keywords,
 *             fill the corresponding fields in the structure with the associated values.
 *             If a line starts with a "#" ignore it, it is a comment.
 * 
 * @param[out] structure The structure to fill.
 * @param      path      The path to the file.
 * @return     0 if the values are successfully retrieved, -1 otherwise.
 */
int load_values_from_file(void *structure, const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        log_error("Error opening file\n");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    size_t structure_element_offset = 0;

    unsigned int read_count = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        char *hash_position = strchr(line, '#');
        if (hash_position != NULL) {
            *hash_position = '\0'; /* Truncate the string at the position of the "#" */
        }

        const char *equal_sign = strchr(line, '=');
        if (equal_sign == NULL) continue;
        
        equal_sign++; /* Move past the '=' character */

        size_t value_char_position = 0;

        /* Extract characters until a space, tab, null terminator, or newline is encountered */
        while (*equal_sign != '\0' && !isspace((unsigned char)*equal_sign)) {
            ((char *)structure)[structure_element_offset + value_char_position] = *equal_sign;
            value_char_position++;
            equal_sign++;
        }
        
        ((char *)structure)[structure_element_offset + value_char_position] = '\0';

        structure_element_offset += value_char_position + 1;
        read_count++;
    }

    /* Clean up and return -1 if any keyword is not found */
    fclose(file);
    return 0;
}

/*
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
*/

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