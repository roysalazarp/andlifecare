#include <libpq-fe.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>

#include "core/core.h"
#include "globals.h"
#include "utils/utils.h"

int query_users(HomeView *home_view_data_buffer);
int query_countries(HomeView *home_view_data_buffer);

int core_view_home(HomeView *home_view_data_buffer) {
    if (query_users(home_view_data_buffer) == -1) {
        return -1;
    }

    if (query_countries(home_view_data_buffer) == -1) {
        return -1;
    }

    return 0;
}

int query_users(HomeView *home_view_data_buffer) {
    char users_query[224];
    if (file_content_to_string(users_query, (size_t)224, "src/core/views/home/users.sql") == -1) {
        return -1;
    }

    PGresult *users_result = PQexec(conn, users_query);

    if (PQresultStatus(users_result) != PGRES_TUPLES_OK) {
        log_error(PQerrorMessage(conn));
        return -1;
    }

    /*
    core_utils_print_query_result(users_result);
    */

    int users_result_columns = PQnfields(users_result);

    home_view_data_buffer->users_data.rows = PQntuples(users_result);
    home_view_data_buffer->users_data.users = (User *)malloc(sizeof(User) * home_view_data_buffer->users_data.rows);
    if (home_view_data_buffer->users_data.users == NULL) {
        log_error("Failed to allocate memory for home_view_data_buffer->users_data.users\n");
        PQclear(users_result);
        return -1;
    }

    int i;
    for (i = 0; i < users_result_columns; ++i) {
        char *column_name = PQfname(users_result, i);
        size_t column_name_length = strlen(column_name);

        strncpy(home_view_data_buffer->users_data.columns[i], column_name, column_name_length);
        home_view_data_buffer->users_data.columns[i][column_name_length] = '\0';
    }

    for (i = 0; i < home_view_data_buffer->users_data.rows; ++i) {
        char *id = PQgetvalue(users_result, i, 0);
        size_t id_length = strlen(id);
        strncpy(home_view_data_buffer->users_data.users[i].id, id, id_length);
        (home_view_data_buffer->users_data.users)[i].id[id_length] = '\0';

        char *email = PQgetvalue(users_result, i, 1);
        size_t email_length = strlen(email);
        strncpy(home_view_data_buffer->users_data.users[i].email, email, email_length);
        home_view_data_buffer->users_data.users[i].email[email_length] = '\0';

        char *country = PQgetvalue(users_result, i, 2);
        size_t country_length = strlen(country);
        strncpy(home_view_data_buffer->users_data.users[i].country, country, country_length);
        home_view_data_buffer->users_data.users[i].country[country_length] = '\0';

        char *full_name = PQgetvalue(users_result, i, 3);
        size_t full_name_length = strlen(full_name);
        strncpy(home_view_data_buffer->users_data.users[i].full_name, full_name, full_name_length);
        home_view_data_buffer->users_data.users[i].full_name[full_name_length] = '\0';
    }

    PQclear(users_result);

    return 0;
}

int query_countries(HomeView *home_view_data_buffer) {
    char countries_query[84];
    if (file_content_to_string(countries_query, (size_t)84, "src/core/views/home/countries.sql") == -1) {
        return -1;
    }

    PGresult *countries_result = PQexec(conn, countries_query);

    if (PQresultStatus(countries_result) != PGRES_TUPLES_OK) {
        log_error(PQerrorMessage(conn));
        return -1;
    }

    /*
    core_utils_print_query_result(countries_result);
    */

    int countries_result_columns = PQnfields(countries_result);

    home_view_data_buffer->countries_data.rows = PQntuples(countries_result);
    home_view_data_buffer->countries_data.countries = (Country *)malloc(sizeof(Country) * home_view_data_buffer->countries_data.rows);
    if (home_view_data_buffer->countries_data.countries == NULL) {
        log_error("Failed to allocate memory for home_view_data_buffer->countries_data.countries\n");
        PQclear(countries_result);
        return -1;
    }
    int i;
    for (i = 0; i < countries_result_columns; ++i) {
        char *column_name = PQfname(countries_result, i);
        size_t column_name_length = strlen(column_name);

        strncpy(home_view_data_buffer->countries_data.columns[i], column_name, column_name_length);
        home_view_data_buffer->countries_data.columns[i][column_name_length] = '\0';
    }

    for (i = 0; i < home_view_data_buffer->countries_data.rows; ++i) {
        char *id = PQgetvalue(countries_result, i, 0);
        int id_number = atoi(id);
        (home_view_data_buffer->countries_data.countries)[i].id = id_number;

        char *country_name = PQgetvalue(countries_result, i, 1);
        size_t country_name_length = strlen(country_name);
        strncpy(home_view_data_buffer->countries_data.countries[i].country_name, country_name, country_name_length);
        home_view_data_buffer->countries_data.countries[i].country_name[country_name_length] = '\0';

        char *iso3 = PQgetvalue(countries_result, i, 2);
        size_t iso3_length = strlen(iso3);
        strncpy(home_view_data_buffer->countries_data.countries[i].iso3, iso3, iso3_length);
        home_view_data_buffer->countries_data.countries[i].iso3[iso3_length] = '\0';
    }

    PQclear(countries_result);

    return 0;
}
