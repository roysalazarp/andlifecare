#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

void te_log_error(const char *message) {
    perror(message);
    fprintf(stderr, "Error code: %d\n", errno);
}

char *te_substring_address_find(const char *substring, const char *string, unsigned int start_from_position, int direction) {
    if (string == NULL || substring == NULL) {
        return NULL;
    }

    size_t string_length = strlen(string) + 1;
    size_t substring_length = strlen(substring); /* we only want characters */

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
            return (char*)(string + i);
        }
    }

    te_log_error("Couldn't find the substring");
    return NULL;
}

/* null-terminates string after modifying */
int te_substring_copy_into_string_at_memory_space(const char *substring, char **string, const char *begin_address, const char *end_address) {
    if (*string == NULL || begin_address == NULL || end_address == NULL || substring == NULL) {
        return -1;
    }

    size_t begin_index = begin_address - *string;
    size_t end_index = end_address - *string;

    if (begin_index >= end_index || end_index > strlen(*string)) {
        te_log_error("Something is wrong with the indexes");
        return -1;
    }

    size_t value_length = strlen(substring); /* we only want characters */
    size_t remaining_string_length = strlen(*string + end_index) + 1;

    char *after;
    after = malloc(remaining_string_length * (sizeof *after));
    if (after == NULL) {
        te_log_error("Failed to re-allocate memory for after\n");
        return -1;
    }

    strncpy(after, *string + end_index, remaining_string_length);
    after[remaining_string_length - 1] = '\0';

    *string = (char*)realloc(*string, (begin_index + value_length + remaining_string_length) * (sizeof **string));
    if (*string == NULL) {
        te_log_error("Failed to re-allocate memory for new_string\n");
        return -1;
    }

    /* TODO: Check for error in memmove and strncpy */
    memmove(*string + begin_index + value_length, after, remaining_string_length);
    strncpy(*string + begin_index, substring, value_length);

    free(after);
    after = NULL;

    return 0;
}


/* Frees dest memory on failure */
int te_string_copy_into_all_buffers(const char *string, char** buffer_array, size_t buffer_array_length) {
    size_t i;
    for (i = 0; i < buffer_array_length; ++i) {
        size_t source_lenght = strlen(string);
        buffer_array[i] = (char*)malloc(source_lenght * sizeof(char) + 1);

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

void te_buffer_array_free(char** buffer_array, size_t buffer_array_length) {
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
    
    int substring_to_remove_position_within_string = substring_to_remove_address - *string;
    
    char *opening_braces_address = te_substring_address_find("{{", *string, substring_to_remove_position_within_string, -1);
    if (opening_braces_address == NULL) {
        return -1;
    }

    char *closing_braces_address = te_substring_address_find("}}", *string, substring_to_remove_position_within_string, 1);
    if (closing_braces_address == NULL) {
        return -1;
    }
    
    if (te_substring_copy_into_string_at_memory_space(substring_to_add, string, opening_braces_address, closing_braces_address + 2) == -1) {
        return -1;
    }

    return 0;
}

int te_multiple_substring_swap(char *block_id, char *substring_to_remove, char **substrings_to_add, char **string) {    
    char *keyword_address = te_substring_address_find(block_id, *string, 0, 1);
    unsigned int keyword_position_within_template = keyword_address - *string;

    char *start_braces_address = te_substring_address_find("{{", *string, keyword_position_within_template, -1);
    char *end_braces_address = te_substring_address_find("}}", *string, keyword_position_within_template, 1);

    const char *prefix = "end ";
    size_t end_keyword_length = strlen(prefix) + strlen(block_id);
    char *end_keyword;
    end_keyword = (char*)malloc(end_keyword_length * (sizeof *end_keyword) + 1);
    if (end_keyword == NULL) {
        te_log_error("Failed to allocate memory for end_keyword\n");
        return -1;
    }

    end_keyword[0] = '\0';

    if (sprintf(end_keyword, "%s%s", prefix, block_id) < 0) {
        te_log_error("Failed to format string\n");
        free(end_keyword);
        end_keyword = NULL;
        return -1;
    }

    char *end_keyword_address = te_substring_address_find(end_keyword, *string, keyword_position_within_template, 1);
    free(end_keyword);
    end_keyword = NULL;

    unsigned int end_keyword_position_within_template = end_keyword_address - *string;

    char *e_start_braces_address = te_substring_address_find("{{", *string, end_keyword_position_within_template, -1);
    char *e_end_braces_address = te_substring_address_find("}}", *string, end_keyword_position_within_template, 1);

    size_t length = e_start_braces_address - (end_braces_address + 2);

    char *for_template;
    for_template = (char*)malloc(length * (sizeof *for_template) + 1);
    if (for_template == NULL) {
        te_log_error("Failed to allocate memory for for_template\n");
        return -1;
    }

    for_template[0] = '\0';

    if (strncpy(for_template, end_braces_address + 2, length) == NULL) {
        te_log_error("Failed copy string\n");
        free(for_template);
        for_template = NULL;
        return -1;
    }

    for_template[length] = '\0';

    size_t for_length = 0;
    
    size_t i;
    for (i = 0; substrings_to_add[i] != NULL; i++) {
        for_length++;
    }

    char** for_items = (char**)malloc(for_length * sizeof(char*));
    if (for_items == NULL) {
        te_log_error("Failed to allocate memory for for_items\n");
        free(for_template);
        for_template = NULL;
        return -1;
    }

    for (i = 0; i < for_length; i++) {
        for_items[i] = NULL;
    }
    
    if (te_string_copy_into_all_buffers(for_template, for_items, for_length) == -1) {
        te_log_error("Failed to make for_items\n");
        free(for_template);
        for_template = NULL;
        return -1;
    }

    free(for_template);
    for_template = NULL;

    for (i = 0; i < for_length; ++i) {
        if (te_single_substring_swap(substring_to_remove, substrings_to_add[i], &for_items[i]) == -1) {
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
    }

    size_t total_length = 0;
    for (i = 0; i < for_length; ++i) {
        total_length += strlen(for_items[i]);
    }

    char *rendered_for;
    rendered_for = (char*)malloc(total_length * (sizeof *rendered_for) + 1);
    if (rendered_for == NULL) {
        te_log_error("Failed to allocate memory for rendered_for\n");
        return -1;
    }

    rendered_for[0] = '\0'; 

    for (i = 0; i < for_length; ++i) {
        /* TODO: Check for strcat errors */
        /* strcat will do its work before the null-terminator */
        strcat(rendered_for, for_items[i]);
    }

    te_buffer_array_free(for_items, for_length);

    if (te_substring_copy_into_string_at_memory_space(rendered_for, string, start_braces_address, e_end_braces_address + 2) == -1) {
        free(rendered_for);
        rendered_for = NULL;
        return -1;
    }

    free(rendered_for);
    return 0;
}