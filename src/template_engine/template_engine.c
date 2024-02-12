#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void te_log_error(const char *message) {
    perror(message);
    fprintf(stderr, "Error code: %d\n", errno);
}

char *te_substring_address_find(const char *substring, const char *string, unsigned int start_from_position, int direction) {
    if (string == NULL || substring == NULL) {
        te_log_error("string or substring is NULL");
        return NULL;
    }

    size_t string_length = strlen(string) + 1; /* count null-terminator as well */
    size_t substring_length = strlen(substring);

    if (substring_length > string_length) {
        te_log_error("substring can't be greater than string");
        return NULL;
    }

    unsigned int end = 0;
    int step = 0;

    if (direction > 0) {
        /* Forward search */
        end = string_length;
        step = 1;
    } else {
        /* Backward search */
        end = 0;
        step = -1;
    }

    unsigned int i;
    for (i = start_from_position; i != end; i += step) {
        if (strncmp(string + i, substring, substring_length) == 0) {
            /* Found the substring, return its address */
            return (char *)(string + i);
        }
    }

    te_log_error("Couldn't find the substring");
    return NULL;
}

/* null-terminates string after modifying */
int te_substring_copy_into_string_at_memory_space(const char *substring, char **string, const char *begin_address, const char *end_address) {
    if (*string == NULL || begin_address == NULL || end_address == NULL || substring == NULL) {
        te_log_error("One of the arguments is NULL");
        return -1;
    }

    size_t begin_index = begin_address - *string;
    size_t end_index = end_address - *string;

    if (begin_index >= end_index || end_index > strlen(*string)) {
        te_log_error("Something is wrong with the indexes");
        return -1;
    }

    size_t value_length = strlen(substring); /* we only want characters */
    size_t remaining_string_length = strlen(((*string) + end_index));

    char *after = NULL;
    after = malloc((remaining_string_length + 1) * (sizeof *after));
    if (after == NULL) {
        te_log_error("Failed to re-allocate memory for after\n");
        return -1;
    }

    memcpy(after, *string + end_index, remaining_string_length + 1);
    after[remaining_string_length] = '\0';

    *string = (char *)realloc(*string, (begin_index + value_length + remaining_string_length + 1) * (sizeof **string));
    if (*string == NULL) {
        te_log_error("Failed to re-allocate memory for new_string\n");
        return -1;
    }

    /* TODO: Check for error in memmove and memcpy */
    memmove(*string + begin_index + value_length, after, remaining_string_length + 1);
    memcpy(*string + begin_index, substring, value_length);

    free(after);
    after = NULL;

    return 0;
}

/* Frees dest memory on failure */
int te_string_copy_into_all_buffers(const char *string, char **buffer_array, size_t buffer_array_length) {
    size_t i;
    for (i = 0; i < buffer_array_length; ++i) {
        size_t source_lenght = strlen(string);
        buffer_array[i] = (char *)malloc(source_lenght * sizeof(char) + 1);

        if (buffer_array[i] == NULL) {
            fprintf(stderr, "Failed to allocate memory for copy %lu\n", (unsigned long)i);

            /* Clean up previously allocated memory */
            size_t j;
            for (j = 0; j < i; ++j) {
                free(buffer_array[j]);
                buffer_array[j] = NULL;
            }

            free(buffer_array);
            buffer_array = NULL;
            return -1;
        }

        buffer_array[i][0] = '\0';
        strcpy(buffer_array[i], string);

        buffer_array[i][source_lenght] = '\0';
    }

    return 0;
}

void te_buffer_array_free(char **buffer_array, size_t buffer_array_length) {
    size_t i;
    for (i = 0; i < buffer_array_length; ++i) {
        free(buffer_array[i]);
        buffer_array[i] = NULL;
    }

    free(buffer_array);
    buffer_array = NULL;
}

int te_single_substring_swap(char *substring_to_remove, char *substring_to_add, char **string) {
    char *substring_to_remove_address = te_substring_address_find(substring_to_remove, *string, 0, 1);
    if (substring_to_remove_address == NULL) {
        return -1;
    }

    if (te_substring_copy_into_string_at_memory_space(substring_to_add, string, substring_to_remove_address, substring_to_remove_address + strlen(substring_to_remove)) == -1) {
        return -1;
    }

    return 0;
}

int te_multiple_substring_swap(char *open_token, char *close_token, size_t number_of_values, char ***substrings_to_add, char **string, size_t number_of_times) {
    char *open_token_address = te_substring_address_find(open_token, *string, 0, 1);
    unsigned int codeblock_start_position = (open_token_address + strlen(open_token)) - *string;

    char *close_token_address = te_substring_address_find(close_token, *string, codeblock_start_position, 1);
    unsigned int codeblock_end_position = close_token_address - *string;

    size_t length = codeblock_end_position - codeblock_start_position;

    char *for_template;
    for_template = (char *)malloc(length * (sizeof *for_template) + 1);
    if (for_template == NULL) {
        te_log_error("Failed to allocate memory for for_template\n");
        return -1;
    }

    for_template[0] = '\0';

    if (memcpy(for_template, open_token_address + strlen(open_token), length) == NULL) {
        te_log_error("Failed copy string\n");
        free(for_template);
        for_template = NULL;
        return -1;
    }

    for_template[length] = '\0';

    char **for_items = (char **)malloc(number_of_times * sizeof(char *));
    if (for_items == NULL) {
        te_log_error("Failed to allocate memory for for_items\n");
        free(for_template);
        for_template = NULL;
        return -1;
    }

    size_t i;
    for (i = 0; i < number_of_times; i++) {
        for_items[i] = NULL;
    }

    if (te_string_copy_into_all_buffers(for_template, for_items, number_of_times) == -1) {
        te_log_error("Failed to make for_items\n");
        free(for_template);
        for_template = NULL;
        return -1;
    }

    free(for_template);
    for_template = NULL;

    size_t j;
    for (i = 0; i < number_of_times; ++i) {
        for (j = 0; j < number_of_values; ++j) {
            char *keyword_start_position = strstr(open_token, "for");

            unsigned int count = 0;
            while ((*keyword_start_position + count) != '\0' && !isspace(((unsigned char)(*(keyword_start_position + count))))) {
                count++;
            }

            char opening_brackets[] = "{{ ";
            char closing_brackets[] = " }}";

            char arrow[] = "->v";

            char num_string[21]; /* number string probably wont be longer than 2 bytes + null-terminator ("99" -> is 3 bytes becuase it would have a null terminator) */
            sprintf(num_string, "%lu", j);
            size_t string_num_length = strlen(num_string);

            char *substring_to_remove;
            size_t substring_to_remove_length = (strlen(opening_brackets) + string_num_length + strlen(arrow) + count + strlen(closing_brackets)) * (sizeof *substring_to_remove);
            substring_to_remove = (char *)malloc(substring_to_remove_length + 1);

            memcpy(substring_to_remove, opening_brackets, strlen(opening_brackets));
            memcpy(substring_to_remove + strlen(opening_brackets), keyword_start_position, count);

            sprintf(substring_to_remove + strlen(opening_brackets) + count, "%s%s%s", arrow, num_string, closing_brackets);
            /* No need to null-terminate after memcpy since sprintf does it */

            if (te_single_substring_swap(substring_to_remove, substrings_to_add[i][j], &for_items[i]) == -1) {
                /* Clean up previously allocated memory */
                size_t j;
                for (j = 0; j < i; ++j) {
                    free(for_items[j]);
                    for_items[j] = NULL;
                }
                free(for_items);
                for_items = NULL;
                return -1;
            }

            free(substring_to_remove);
            substring_to_remove = NULL;
        }
    }

    size_t total_length = 0;
    for (i = 0; i < number_of_times; ++i) {
        total_length += strlen(for_items[i]);
    }

    char *rendered_for;
    rendered_for = (char *)malloc(total_length * (sizeof *rendered_for) + 1);
    if (rendered_for == NULL) {
        te_log_error("Failed to allocate memory for rendered_for\n");
        return -1;
    }

    rendered_for[0] = '\0';

    for (i = 0; i < number_of_times; ++i) {
        /* TODO: Check for strcat errors */
        /* strcat will do its work before the null-terminator */
        strcat(rendered_for, for_items[i]);
    }

    te_buffer_array_free(for_items, number_of_times);

    if (te_substring_copy_into_string_at_memory_space(rendered_for, string, open_token_address, close_token_address + strlen(close_token)) == -1) {
        free(rendered_for);
        rendered_for = NULL;
        return -1;
    }

    free(rendered_for);
    return 0;
}