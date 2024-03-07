int te_single_substring_swap(char *substring_to_remove, char *substring_to_add, char **string);
int te_copy_substring_block(char **buffer, size_t tokens_positions[2], char *opening_token, char *closing_token, char **string);
int te_multiple_substring_swap(char *opening_token, char *closing_token, size_t number_of_values, char ***substrings_to_add, char **string, size_t number_of_times);
char *te_substring_location_find(const char *substring, const char *string, size_t start_from_position, short direction);
int te_substring_copy_into_string_at_memory_space(const char *substring, char **string, const char *begin_address, const char *end_address);
