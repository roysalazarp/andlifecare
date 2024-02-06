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
    int rows;
} UsersData;

typedef struct {
    int id;
    char country_name[80];
    char iso3[4];
} Country;

typedef struct {
    Country *countries;
    char columns[ROWS][13]; /* 'country_name' the current largest column name contains 12 characters */
    int rows;
} CountriesData;

typedef struct {
    UsersData users_data;
    CountriesData countries_data;
} HomeView;

int core_view_home(HomeView *home_view_data_buffer);

void core_utils_view_home_free(HomeView *home_view_data);
void core_utils_print_query_result(PGresult *result);

#endif