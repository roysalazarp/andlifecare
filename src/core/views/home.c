#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <libpq-fe.h>

#include "globals.h"
#include "utils/utils.h"
#include "core/core.h"

int core_view_home(User **users, int *num_rows, int *num_columns) {
    char query[213];
    size_t query_length = 213;
    if (file_content_to_string(query, query_length, "/src/core/views/home.sql") == -1) {
        return -1;
    }

    PGresult *result = PQexec(conn, query);
    
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        log_error(PQerrorMessage(conn));
        return -1;
    }
    
    *num_rows = PQntuples(result);
    *num_columns = PQnfields(result);

    *users = malloc(sizeof(User) * (*num_rows));

    int i;
    for (i = 0; i < *num_rows; ++i) {
        strncpy((*users)[i].id, PQgetvalue(result, i, 0), sizeof((*users)[i].id)-1);
        (*users)[i].id[sizeof((*users)[i].id)-1] = '\0';

        strncpy((*users)[i].email, PQgetvalue(result, i, 1), sizeof((*users)[i].email)-1);
        (*users)[i].email[sizeof((*users)[i].email)-1] = '\0';

        strncpy((*users)[i].country, PQgetvalue(result, i, 2), sizeof((*users)[i].country)-1);
        (*users)[i].country[sizeof((*users)[i].country)-1] = '\0';
        
        strncpy((*users)[i].full_name, PQgetvalue(result, i, 3), sizeof((*users)[i].full_name)-1);
        (*users)[i].full_name[sizeof((*users)[i].full_name)-1] = '\0';
    }

    PQclear(result);

    return 0;
}