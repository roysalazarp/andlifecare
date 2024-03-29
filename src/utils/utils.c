#include <ctype.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 100

ssize_t calculate_file_size(char *absolute_file_path) {
    FILE *file = fopen(absolute_file_path, "r");

    if (file == NULL) {
        fprintf(stderr, "Failed to open file\nError code: %d\n", errno);
        return -1;
    }

    if (fseek(file, 0, SEEK_END) == -1) {
        fprintf(stderr, "Failed to move the 'file position indicator' to the end of the file\nError code: %d\n", errno);
        fclose(file);
        return -1;
    }

    long file_size = ftell(file);
    if (file_size == -1) {
        fprintf(stderr, "Failed to determine the current 'file position indicator' of the file\nError code: %d\n", errno);
        fclose(file);
        return -1;
    }

    rewind(file);
    fclose(file);

    return file_size;
}

int read_file(char *buffer, char *absolute_file_path, size_t file_size) {
    FILE *file = fopen(absolute_file_path, "r");

    if (file == NULL) {
        fprintf(stderr, "Failed to open file\nError code: %d\n", errno);
        return -1;
    }

    if (fread(buffer, sizeof(char), file_size, file) != file_size) {
        if (feof(file)) {
            fprintf(stderr, "End of file reached before reading all elements\nError code: %d\n", errno);
        }

        if (ferror(file)) {
            fprintf(stderr, "An error occurred during the fread operation\nError code: %d\n", errno);
        }

        fclose(file);
        return -1;
    }

    fclose(file);

    return 0;
}

int build_absolute_path(char *absolute_path_buffer, const char *path_relative_to_project_root) {
    char *cwd;
    cwd = (char *)malloc(PATH_MAX * (sizeof *cwd));
    if (cwd == NULL) {
        fprintf(stderr, "Failed to allocate memory for cwd\nError code: %d\n", errno);
        return -1;
    }

    if (getcwd(cwd, PATH_MAX) == NULL) {
        fprintf(stderr, "Failed to get current working directory\nError code: %d\n", errno);
        free(cwd);
        cwd = NULL;
        return -1;
    }

    if (sprintf(absolute_path_buffer, "%s/%s", cwd, path_relative_to_project_root) < 0) {
        fprintf(stderr, "Absolute path truncated\nError code: %d\n", errno);
        free(cwd);
        cwd = NULL;
        return -1;
    }

    free(cwd);
    cwd = NULL;

    return 0;
}

int read_file_from_path_relative_to_project_root(char **buffer, const char *file_path_relative_to_project_root) {
    char *file_absolute_path;
    file_absolute_path = (char *)malloc(PATH_MAX * (sizeof *file_absolute_path) + 1);
    if (file_absolute_path == NULL) {
        fprintf(stderr, "Failed to allocate memory for file_absolute_path\nError code: %d\n", errno);
        return -1;
    }

    file_absolute_path[0] = '\0';

    if (build_absolute_path(file_absolute_path, file_path_relative_to_project_root) == -1) {
        free(file_absolute_path);
        file_absolute_path = NULL;
        return -1;
    }

    ssize_t file_size = calculate_file_size(file_absolute_path);

    *buffer = (char *)malloc(file_size * (sizeof **buffer) + 1);
    if (*buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for *buffer\nError code: %d\n", errno);
        free(file_absolute_path);
        file_absolute_path = NULL;
        return -1;
    }

    if (read_file(*buffer, file_absolute_path, file_size) == -1) {
        free(file_absolute_path);
        file_absolute_path = NULL;
        free(*buffer);
        *buffer = NULL;
        return -1;
    }

    (*buffer)[file_size] = '\0';

    free(file_absolute_path);
    file_absolute_path = NULL;

    return 0;
}

/**
 * @brief      Read values from a file into a structure. Ignores lines that start with '#' (comments).
 *             The values in the file must adhere to the following rules:
 *                  1. Each value must be on a new line.
 *                  2. Values must adhere to the specified order and length defined in the structure
 *                     they are loaded into.
 *                  3. values must start inmediately after the equal sign and be contiguous.
 *                     e.g. =helloworld
 *
 * @param[out] structure A structure with fixed-size string fields for each value in the file.
 * @param      file_path_relative_to_project_root The path to the file from project root.
 * @return     The amount of values read if success, -1 otherwise.
 */
int load_values_from_file(void *structure, const char *file_path_relative_to_project_root) {
    char *file_absolute_path;
    file_absolute_path = (char *)malloc(PATH_MAX * (sizeof *file_absolute_path) + 1);
    if (file_absolute_path == NULL) {
        fprintf(stderr, "Failed to allocate memory for file_absolute_path\nError code: %d\n", errno);
        return -1;
    }

    file_absolute_path[0] = '\0';

    if (build_absolute_path(file_absolute_path, file_path_relative_to_project_root) == -1) {
        free(file_absolute_path);
        file_absolute_path = NULL;
        return -1;
    }

    FILE *file = fopen(file_absolute_path, "r");

    free(file_absolute_path);
    file_absolute_path = NULL;

    if (file == NULL) {
        fprintf(stderr, "Failed to open file\nError code: %d\n", errno);
        return -1;
    }

    char line[MAX_LINE_LENGTH];

    size_t structure_element_offset = 0;

    unsigned int read_values_count = 0;

    while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
        /** Does line have a comment? */
        char *hash_position = strchr(line, '#');
        if (hash_position != NULL) {
            /** We don't care about comments. Truncate line at the start of a comment */
            *hash_position = '\0';
        }

        /** The value we want to read starts after an '=' sign */
        char *equal_sign = strchr(line, '=');
        if (equal_sign == NULL) {
            /** This line does not contain a value, continue to next line */
            continue;
        }

        equal_sign++; /** Move the pointer past the '=' character to the beginning of the value */

        size_t value_char_index = 0;

        /** Extract 'value characters' until a 'whitespace' character or null-terminator is encountered */
        while (*equal_sign != '\0' && !isspace((unsigned char)*equal_sign)) {
            ((char *)structure)[structure_element_offset + value_char_index] = *equal_sign;
            value_char_index++;
            equal_sign++;
        }

        ((char *)structure)[structure_element_offset + value_char_index] = '\0';

        /** Next element in the structure should start after the read value null-terminator */
        structure_element_offset += value_char_index + 1;
        read_values_count++;
    }

    fclose(file);
    return read_values_count;
}
