#include <libpq-fe.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>

#include "core/core.h"
#include "globals.h"
#include "queue.h"
#include "utils/utils.h"

int query_users(UiTestView *ui_test_view_data_buffer, PGconn *conn);
int query_countries(UiTestView *ui_test_view_data_buffer, PGconn *conn);

int core_view_ui_test(UiTestView *ui_test_view_data_buffer, int client_socket, int conn_index) {
    PGconn *conn = conn_pool[conn_index];

    if (PQstatus(conn) != CONNECTION_OK) {
        log_error(PQerrorMessage(conn));
        exit(EXIT_FAILURE);
    }

    if (query_users(ui_test_view_data_buffer, conn) == -1) {
        return -1;
    }

    if (query_countries(ui_test_view_data_buffer, conn) == -1) {
        return -1;
    }

    return 0;
}

int query_users(UiTestView *ui_test_view_data_buffer, PGconn *conn) {
    char *query_absolute_path;
    query_absolute_path = (char *)malloc(PATH_MAX * (sizeof *query_absolute_path) + 1);
    if (query_absolute_path == NULL) {
        log_error("Failed to allocate memory for query_absolute_path\n");
        return -1;
    }

    query_absolute_path[0] = '\0';

    if (build_absolute_path(query_absolute_path, "src/core/views/ui_test/users.sql") == -1) {
        free(query_absolute_path);
        query_absolute_path = NULL;
        return -1;
    }

    long query_file_size = calculate_file_size(query_absolute_path);

    char *users_query;
    users_query = (char *)malloc(query_file_size * (sizeof *users_query) + 1);
    if (read_file(users_query, query_absolute_path, query_file_size) == -1) {
        free(query_absolute_path);
        query_absolute_path = NULL;
        free(users_query);
        users_query = NULL;
        return -1;
    }

    users_query[query_file_size] = '\0';

    PGresult *users_result = PQexec(conn, users_query);

    free(query_absolute_path);
    query_absolute_path = NULL;
    free(users_query);
    users_query = NULL;

    if (PQresultStatus(users_result) != PGRES_TUPLES_OK) {
        log_error(PQerrorMessage(conn));
        return -1;
    }

    /*
    core_utils_print_query_result(users_result);
    */

    int users_result_columns = PQnfields(users_result);

    ui_test_view_data_buffer->users_data.rows = PQntuples(users_result);
    ui_test_view_data_buffer->users_data.users = (User *)malloc(sizeof(User) * ui_test_view_data_buffer->users_data.rows);
    if (ui_test_view_data_buffer->users_data.users == NULL) {
        log_error("Failed to allocate memory for ui_test_view_data_buffer->users_data.users\n");
        PQclear(users_result);
        return -1;
    }

    int i;
    for (i = 0; i < users_result_columns; ++i) {
        char *column_name = PQfname(users_result, i);
        size_t column_name_length = strlen(column_name);

        memcpy(ui_test_view_data_buffer->users_data.columns[i], column_name, column_name_length);
        ui_test_view_data_buffer->users_data.columns[i][column_name_length] = '\0';
    }

    for (i = 0; i < ui_test_view_data_buffer->users_data.rows; ++i) {
        char *id = PQgetvalue(users_result, i, 0);
        size_t id_length = strlen(id);
        memcpy(ui_test_view_data_buffer->users_data.users[i].id, id, id_length);
        (ui_test_view_data_buffer->users_data.users)[i].id[id_length] = '\0';

        char *email = PQgetvalue(users_result, i, 1);
        size_t email_length = strlen(email);
        memcpy(ui_test_view_data_buffer->users_data.users[i].email, email, email_length);
        ui_test_view_data_buffer->users_data.users[i].email[email_length] = '\0';

        char *country = PQgetvalue(users_result, i, 2);
        size_t country_length = strlen(country);
        memcpy(ui_test_view_data_buffer->users_data.users[i].country, country, country_length);
        ui_test_view_data_buffer->users_data.users[i].country[country_length] = '\0';

        char *full_name = PQgetvalue(users_result, i, 3);
        size_t full_name_length = strlen(full_name);
        memcpy(ui_test_view_data_buffer->users_data.users[i].full_name, full_name, full_name_length);
        ui_test_view_data_buffer->users_data.users[i].full_name[full_name_length] = '\0';
    }

    PQclear(users_result);

    return 0;
}

int query_countries(UiTestView *ui_test_view_data_buffer, PGconn *conn) {
    char *query_absolute_path;
    query_absolute_path = (char *)malloc(PATH_MAX * (sizeof *query_absolute_path) + 1);
    if (query_absolute_path == NULL) {
        log_error("Failed to allocate memory for query_absolute_path\n");
        return -1;
    }

    query_absolute_path[0] = '\0';

    if (build_absolute_path(query_absolute_path, "src/core/views/ui_test/countries.sql") == -1) {
        free(query_absolute_path);
        query_absolute_path = NULL;
        return -1;
    }

    long query_file_size = calculate_file_size(query_absolute_path);

    char *countries_query;
    countries_query = (char *)malloc(query_file_size * (sizeof *countries_query) + 1);
    if (read_file(countries_query, query_absolute_path, query_file_size) == -1) {
        free(query_absolute_path);
        query_absolute_path = NULL;
        free(countries_query);
        countries_query = NULL;
        return -1;
    }

    countries_query[query_file_size] = '\0';

    PGresult *countries_result = PQexec(conn, countries_query);

    free(query_absolute_path);
    query_absolute_path = NULL;
    free(countries_query);
    countries_query = NULL;

    if (PQresultStatus(countries_result) != PGRES_TUPLES_OK) {
        log_error(PQerrorMessage(conn));
        return -1;
    }

    /*
    core_utils_print_query_result(countries_result);
    */

    int countries_result_columns = PQnfields(countries_result);

    ui_test_view_data_buffer->countries_data.rows = PQntuples(countries_result);
    ui_test_view_data_buffer->countries_data.countries = (Country *)malloc(sizeof(Country) * ui_test_view_data_buffer->countries_data.rows);
    if (ui_test_view_data_buffer->countries_data.countries == NULL) {
        log_error("Failed to allocate memory for ui_test_view_data_buffer->countries_data.countries\n");
        PQclear(countries_result);
        return -1;
    }
    int i;
    for (i = 0; i < countries_result_columns; ++i) {
        char *column_name = PQfname(countries_result, i);
        size_t column_name_length = strlen(column_name);

        memcpy(ui_test_view_data_buffer->countries_data.columns[i], column_name, column_name_length);
        ui_test_view_data_buffer->countries_data.columns[i][column_name_length] = '\0';
    }

    for (i = 0; i < ui_test_view_data_buffer->countries_data.rows; ++i) {
        char *id = PQgetvalue(countries_result, i, 0);
        int id_number = atoi(id);
        (ui_test_view_data_buffer->countries_data.countries)[i].id = id_number;

        char *country_name = PQgetvalue(countries_result, i, 1);
        size_t country_name_length = strlen(country_name);
        memcpy(ui_test_view_data_buffer->countries_data.countries[i].country_name, country_name, country_name_length);
        ui_test_view_data_buffer->countries_data.countries[i].country_name[country_name_length] = '\0';

        char *iso3 = PQgetvalue(countries_result, i, 2);
        size_t iso3_length = strlen(iso3);
        memcpy(ui_test_view_data_buffer->countries_data.countries[i].iso3, iso3, iso3_length);
        ui_test_view_data_buffer->countries_data.countries[i].iso3[iso3_length] = '\0';
    }

    PQclear(countries_result);

    return 0;
}
