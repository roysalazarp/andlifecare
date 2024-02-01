int te_single_substring_swap(char *substring_to_remove, char *substring_to_add, char **string);
int te_multiple_substring_swap(char *open_token, char *close_token, size_t number_of_values, char ***substrings_to_add, char **string, size_t number_of_times);
char *te_substring_address_find(const char *substring, const char *string, unsigned int start_from_position, int direction);
int te_substring_copy_into_string_at_memory_space(const char *substring, char **string, const char *begin_address, const char *end_address);
