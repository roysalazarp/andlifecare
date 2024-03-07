#include <errno.h>
#include <libpq-fe.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/core.h"
#include "globals.h"
#include "utils/utils.h"

int query_users(UiTestResult *result, PGconn *conn);
int query_countries(UiTestResult *result, PGconn *conn);

int core_ui_test(UiTestResult *result, int client_socket, int conn_index) {
    PGconn *conn = conn_pool[conn_index];

    if (query_users(result, conn) == -1) {
        return -1;
    }

    if (query_countries(result, conn) == -1) {
        return -1;
    }

    return 0;
}

int query_users(UiTestResult *result, PGconn *conn) {
    const char query[] = "SELECT u.id, u.email, c.nicename AS country, CONCAT(ui.first_name, ' ', ui.last_name) AS full_name FROM app.users u JOIN app.users_info ui ON u.id = ui.user_id JOIN app.countries c ON ui.country_id = c.id";
    PGresult *users_result = PQexec(conn, query);

    if (PQresultStatus(users_result) != PGRES_TUPLES_OK) {
        fprintf(stderr, "%s\nError code: %d\n", PQerrorMessage(conn), errno);
        return -1;
    }

    /* core_utils_print_query_result(users_result); */

    unsigned int users_result_columns = PQnfields(users_result);

    result->users_data.rows = PQntuples(users_result);

    unsigned int i;
    for (i = 0; i < users_result_columns; ++i) {
        char *column_name = PQfname(users_result, i);
        size_t column_name_length = strlen(column_name);

        if (memcpy(result->users_data.columns[i], column_name, column_name_length) == NULL) {
            fprintf(stderr, "Failed to copy column_name into memory buffer\nError code: %d\n", errno);
            return -1;
        }

        result->users_data.columns[i][column_name_length] = '\0';
    }

    result->users_data.users = (User *)malloc(result->users_data.rows * sizeof(User));
    if (result->users_data.users == NULL) {
        fprintf(stderr, "Failed to allocate memory for result->users_data.users\nError code: %d\n", errno);
        PQclear(users_result);
        return -1;
    }

    for (i = 0; i < result->users_data.rows; ++i) {
        char *id = PQgetvalue(users_result, i, 0);
        size_t id_length = strlen(id);
        if (memcpy(result->users_data.users[i].id, id, id_length) == NULL) {
            fprintf(stderr, "Failed to copy id into memory buffer\nError code: %d\n", errno);
            return -1;
        }
        (result->users_data.users)[i].id[id_length] = '\0';

        char *email = PQgetvalue(users_result, i, 1);
        size_t email_length = strlen(email);
        if (memcpy(result->users_data.users[i].email, email, email_length) == NULL) {
            fprintf(stderr, "Failed to copy email into memory buffer\nError code: %d\n", errno);
            return -1;
        }
        result->users_data.users[i].email[email_length] = '\0';

        char *country = PQgetvalue(users_result, i, 2);
        size_t country_length = strlen(country);
        if (memcpy(result->users_data.users[i].country, country, country_length) == NULL) {
            fprintf(stderr, "Failed to copy country into memory buffer\nError code: %d\n", errno);
            return -1;
        }
        result->users_data.users[i].country[country_length] = '\0';

        char *full_name = PQgetvalue(users_result, i, 3);
        size_t full_name_length = strlen(full_name);
        if (memcpy(result->users_data.users[i].full_name, full_name, full_name_length) == NULL) {
            fprintf(stderr, "Failed to copy full_name into memory buffer\nError code: %d\n", errno);
            return -1;
        }
        result->users_data.users[i].full_name[full_name_length] = '\0';
    }

    PQclear(users_result);

    return 0;
}

int query_countries(UiTestResult *result, PGconn *conn) {
    const char query[] = "SELECT c.id, c.nicename AS country_name, c.iso3 FROM app.countries c";
    PGresult *countries_result = PQexec(conn, query);

    if (PQresultStatus(countries_result) != PGRES_TUPLES_OK) {
        fprintf(stderr, "%s\nError code: %d\n", PQerrorMessage(conn), errno);
        return -1;
    }

    /* core_utils_print_query_result(countries_result); */

    unsigned int countries_result_columns = PQnfields(countries_result);

    result->countries_data.rows = PQntuples(countries_result);

    unsigned int i;
    for (i = 0; i < countries_result_columns; ++i) {
        char *column_name = PQfname(countries_result, i);
        size_t column_name_length = strlen(column_name);

        if (memcpy(result->countries_data.columns[i], column_name, column_name_length) == NULL) {
            fprintf(stderr, "Failed to copy column_name into memory buffer\nError code: %d\n", errno);
            return -1;
        }
        result->countries_data.columns[i][column_name_length] = '\0';
    }

    result->countries_data.countries = (Country *)malloc(sizeof(Country) * result->countries_data.rows);
    if (result->countries_data.countries == NULL) {
        fprintf(stderr, "Failed to allocate memory for result->countries_data.countries\nError code: %d\n", errno);
        PQclear(countries_result);
        return -1;
    }

    for (i = 0; i < result->countries_data.rows; ++i) {
        char *id = PQgetvalue(countries_result, i, 0);
        int id_number = atoi(id);
        (result->countries_data.countries)[i].id = id_number;

        char *country_name = PQgetvalue(countries_result, i, 1);
        size_t country_name_length = strlen(country_name);
        if (memcpy(result->countries_data.countries[i].country_name, country_name, country_name_length) == NULL) {
            fprintf(stderr, "Failed to copy country_name into memory buffer\nError code: %d\n", errno);
            return -1;
        }
        result->countries_data.countries[i].country_name[country_name_length] = '\0';

        char *iso3 = PQgetvalue(countries_result, i, 2);
        size_t iso3_length = strlen(iso3);
        if (memcpy(result->countries_data.countries[i].iso3, iso3, iso3_length) == NULL) {
            fprintf(stderr, "Failed to copy iso3 into memory buffer\nError code: %d\n", errno);
            return -1;
        }
        result->countries_data.countries[i].iso3[iso3_length] = '\0';
    }

    PQclear(countries_result);

    return 0;
}
