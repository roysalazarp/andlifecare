#ifndef CORE_H
#define CORE_H

#include <libpq-fe.h>

#define ROWS 4
#define POSTGRES_MAX_COLUMN_NAME_LENGTH 64

typedef struct {
    char id[37]; /* uuid contains 36 characters */
    char email[255];
    char country[80];
    char full_name[255];
} User;
typedef struct {
    User *users;
    char columns[ROWS][10]; /* 'full_name' the current largest column name contains 9 characters */
    unsigned int rows;
} UsersData;

typedef struct {
    int id;
    char country_name[80];
    char iso3[4];
} Country;

typedef struct {
    Country *countries;
    char columns[ROWS][13]; /* 'country_name' the current largest column name contains 12 characters */
    unsigned int rows;
} CountriesData;

typedef struct {
    UsersData users_data;
    CountriesData countries_data;
} UiTestResult;

int core_ui_test(UiTestResult *result, int client_socket, int conn_index);

void core_utils_ui_test_free(UiTestResult *result);
void core_utils_print_query_result(PGresult *query_result);

typedef struct {
    char *email;
    char *password;
    char *repeat_password;
} SignUpCreateUserInput;

typedef struct {
    char session_id[37];
} SignUpCreateUserResult;

int core_sign_up_create_user(SignUpCreateUserResult *result, SignUpCreateUserInput *input, int client_socket, int conn_index);

#endif