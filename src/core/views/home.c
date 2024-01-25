#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <libpq-fe.h>

#include "globals.h"
#include "utils/utils.h"
#include "core/core.h"

int core_view_home(User **user, int *numRows) {
    char query[168];
    size_t query_length = 168;
    if (file_content_to_string(query, query_length, "/src/core/views/home.sql") == -1) {
        return -1;
    }

    PGresult *result = PQexec(conn, query);
    
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        log_error(PQerrorMessage(conn));
        return -1;
    }
    
    *numRows = PQntuples(result);

    *user = malloc(sizeof(User) * (*numRows));

    int i;
    for (i = 0; i < *numRows; ++i) {
        strncpy((*user)[i].id, PQgetvalue(result, i, 0), sizeof((*user)[i].id)-1);
        (*user)[i].id[sizeof((*user)[i].id)-1] = '\0';

        strncpy((*user)[i].email, PQgetvalue(result, i, 1), sizeof((*user)[i].email)-1);
        (*user)[i].email[sizeof((*user)[i].email)-1] = '\0';

        strncpy((*user)[i].first_name, PQgetvalue(result, i, 2), sizeof((*user)[i].first_name)-1);
        (*user)[i].first_name[sizeof((*user)[i].first_name)-1] = '\0';

        strncpy((*user)[i].last_name, PQgetvalue(result, i, 3), sizeof((*user)[i].last_name)-1);
        (*user)[i].last_name[sizeof((*user)[i].last_name)-1] = '\0';

        strncpy((*user)[i].country, PQgetvalue(result, i, 4), sizeof((*user)[i].country)-1);
        (*user)[i].country[sizeof((*user)[i].country)-1] = '\0';
    }

    PQclear(result);

    return 0;
}