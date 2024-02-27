#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *te_substring_location_find(const char *substring, const char *string, size_t start_from_position, short direction) {
    size_t string_length = strlen(string) + 1; /** Count null-terminator as well */
    size_t substring_length = strlen(substring);

    size_t end = 0;
    short step = 0;

    if (direction > 0) {
        /** Forward search */
        end = string_length;
        step = 1;
    } else {
        /** Backward search */
        end = 0;
        step = -1;
    }

    size_t i;
    for (i = start_from_position; i != end; i += step) {
        if (strncmp(string + i, substring, substring_length) == 0) {
            /** Found the substring, return its address */
            return (char *)(string + i);
        }
    }

    fprintf(stderr, "Couldn't find the substring\nError code: %d\n", errno);
    return NULL;
}

int te_substring_copy_into_string_at_memory_space(const char *substring, char **string, const char *begin_address, const char *end_address) {
    size_t begin_index = begin_address - *string;
    size_t end_index = end_address - *string;

    if (begin_index >= end_index) {
        fprintf(stderr, "begin_address can not be after end_address\nError code: %d\n", errno);
        return -1;
    }

    size_t substring_length = strlen(substring);
    size_t remaining_string_length = strlen(((*string) + end_index));

    char *after = NULL;
    after = malloc(remaining_string_length * (sizeof *after) + 1);
    if (after == NULL) {
        fprintf(stderr, "Failed to re-allocate memory for after\nError code: %d\n", errno);
        return -1;
    }

    if (memcpy(after, *string + end_index, remaining_string_length + 1) == NULL) {
        fprintf(stderr, "Failed to copy remaining string into memory buffer\nError code: %d\n", errno);
        free(after);
        after = NULL;
        return -1;
    }

    after[remaining_string_length] = '\0';

    *string = (char *)realloc(*string, (begin_index + substring_length + remaining_string_length + 1) * (sizeof **string));
    if (*string == NULL) {
        fprintf(stderr, "Failed to re-allocate memory for new_string\nError code: %d\n", errno);
        return -1;
    }

    /* TODO: Check for memmove and memcpy errors */
    memmove(*string + begin_index + substring_length, after, remaining_string_length + 1);
    memcpy(*string + begin_index, substring, substring_length);

    free(after);
    after = NULL;

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

int te_string_copy_into_all_buffers(const char *string, char **buffer_array, size_t buffer_array_length) {
    size_t i;
    for (i = 0; i < buffer_array_length; ++i) {
        buffer_array[i] = NULL;
    }

    for (i = 0; i < buffer_array_length; ++i) {
        size_t string_lenght = strlen(string);
        buffer_array[i] = (char *)malloc(string_lenght * sizeof(char) + 1);

        if (buffer_array[i] == NULL) {
            fprintf(stderr, "Failed to allocate memory for copy %lu\nError code: %d\n", (unsigned long)i, errno);
            te_buffer_array_free(buffer_array, (i + 1));
            return -1;
        }

        if (memcpy(buffer_array[i], string, string_lenght) == NULL) {
            fprintf(stderr, "Failed to copy string into memory buffer at iteration %lu\nError code: %d\n", (unsigned long)i, errno);
            te_buffer_array_free(buffer_array, (i + 1));
            return -1;
        }

        buffer_array[i][string_lenght] = '\0';
    }

    return 0;
}

int te_single_substring_swap(char *substring_to_remove, char *substring_to_add, char **string) {
    char *substring_to_remove_address = te_substring_location_find(substring_to_remove, *string, 0, 1);
    if (substring_to_remove_address == NULL) {
        return -1;
    }

    if (te_substring_copy_into_string_at_memory_space(substring_to_add, string, substring_to_remove_address, substring_to_remove_address + strlen(substring_to_remove)) == -1) {
        return -1;
    }

    return 0;
}

/* TODO: This function needs simplifying, it is quite difficult to follow */
int te_multiple_substring_swap(char *opening_token, char *closing_token, size_t number_of_values, char ***substrings_to_add, char **string, size_t number_of_times) {
    /** 1. Find out the length of the relevant 'reference block' inside the string */
    char *open_token_address = te_substring_location_find(opening_token, *string, 0, 1);
    size_t block_start_position = (open_token_address + strlen(opening_token)) - *string;

    char *close_token_address = te_substring_location_find(closing_token, *string, block_start_position, 1);
    size_t block_end_position = close_token_address - *string;

    size_t length = block_end_position - block_start_position;

    /** 2. Make a copy of 'reference block' (we'll call it, the 'working block') */
    char *working_block;
    working_block = (char *)malloc(length * (sizeof *working_block) + 1);
    if (working_block == NULL) {
        fprintf(stderr, "Failed to allocate memory for working_block\nError code: %d\n", errno);
        return -1;
    }

    working_block[0] = '\0';

    if (memcpy(working_block, open_token_address + strlen(opening_token), length) == NULL) {
        fprintf(stderr, "Failed copy string\nError code: %d\n", errno);
        free(working_block);
        working_block = NULL;
        return -1;
    }

    working_block[length] = '\0';

    /** 3. Make multiple copies of the 'working block', (we'll call it, the 'working copies')  */
    char **working_copies = (char **)malloc(number_of_times * sizeof(char *));
    if (working_copies == NULL) {
        fprintf(stderr, "Failed to allocate memory for working_copies\nError code: %d\n", errno);
        free(working_block);
        working_block = NULL;
        return -1;
    }

    if (te_string_copy_into_all_buffers(working_block, working_copies, number_of_times) == -1) {
        fprintf(stderr, "Failed to make working_copies\nError code: %d\n", errno);
        free(working_block);
        working_block = NULL;
        return -1;
    }

    free(working_block);
    working_block = NULL;

    /** 4. Render 'working copies' with the values provided at 'substrings_to_add' */
    size_t i;
    size_t j;
    for (i = 0; i < number_of_times; ++i) {
        for (j = 0; j < number_of_values; ++j) {
            /**
             * 4.1 Render iterating working copy:
             *     We expect &working_copies[i] to point to a string that contains a substring we need to replace
             *     with the value of substrings_to_add[i][j]. The substring we're looking to replace resembles the
             *     opening_token.
             *
             *     For instance, if the opening_token is "{{ for->some_value }}", the substring to replace would be
             *     "{{ for->some_value->v(n) }}", where 'n' represents the current iteration j.
             *
             *     We recreate such a substring and we pass it to te_single_substring_swap, which searches for it
             *     inside &working_copies[i] and swaps it for the value of substrings_to_add[i][j].
             */
            char *keyword_start_position;
            if ((keyword_start_position = strstr(opening_token, "for")) == NULL) {
                fprintf(stderr, "Failed to find string\nError code: %d\n", errno);
                te_buffer_array_free(working_copies, (i + 1));
                return -1;
            }

            size_t count = 0;
            while ((*keyword_start_position + count) != '\0' && !isspace(((unsigned char)(*(keyword_start_position + count))))) {
                count++;
            }

            char opening_brackets[] = "{{ ";
            char closing_brackets[] = " }}";

            char arrow[] = "->v";

            char num_as_string[21];
            sprintf(num_as_string, "%lu", j);
            size_t num_as_string_length = strlen(num_as_string);

            char *substring_to_remove;
            size_t substring_to_remove_length = (strlen(opening_brackets) + num_as_string_length + strlen(arrow) + count + strlen(closing_brackets)) * (sizeof *substring_to_remove);
            substring_to_remove = (char *)malloc(substring_to_remove_length + 1);

            memcpy(substring_to_remove, opening_brackets, strlen(opening_brackets));
            memcpy(substring_to_remove + strlen(opening_brackets), keyword_start_position, count);

            sprintf(substring_to_remove + strlen(opening_brackets) + count, "%s%s%s", arrow, num_as_string, closing_brackets);
            /* No need to null-terminate after memcpy since sprintf does it */

            if (te_single_substring_swap(substring_to_remove, substrings_to_add[i][j], &working_copies[i]) == -1) {
                te_buffer_array_free(working_copies, (i + 1));
                free(substring_to_remove);
                substring_to_remove = NULL;
                return -1;
            }

            free(substring_to_remove);
            substring_to_remove = NULL;
        }
    }

    /** 5. Combine all 'rendered working copies' */
    size_t total_length = 0;
    for (i = 0; i < number_of_times; ++i) {
        total_length += strlen(working_copies[i]);
    }

    char *rendered_working_copies;
    rendered_working_copies = (char *)malloc(total_length * (sizeof *rendered_working_copies) + 1);
    if (rendered_working_copies == NULL) {
        fprintf(stderr, "Failed to allocate memory for rendered_working_copies\nError code: %d\n", errno);
        return -1;
    }

    rendered_working_copies[0] = '\0';

    for (i = 0; i < number_of_times; ++i) {
        /* TODO: Check for strcat errors */
        /* strcat will do its work before the null-terminator */
        strcat(rendered_working_copies, working_copies[i]);
    }

    te_buffer_array_free(working_copies, number_of_times);

    /** 6. Swap 'rendered working copies' into specified 'reference block' */
    if (te_substring_copy_into_string_at_memory_space(rendered_working_copies, string, open_token_address, close_token_address + strlen(closing_token)) == -1) {
        free(rendered_working_copies);
        rendered_working_copies = NULL;
        return -1;
    }

    free(rendered_working_copies);
    return 0;
}