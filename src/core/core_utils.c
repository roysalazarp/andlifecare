#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>

#include "core/core.h"

void core_utils_view_home_free(HomeView *home_view_data_buffer) {
    free(home_view_data_buffer->users_data.users);
    home_view_data_buffer->users_data.users = NULL;
    free(home_view_data_buffer->countries_data.countries);
    home_view_data_buffer->countries_data.countries = NULL;
}

void core_utils_print_query_result(PGresult *result) {
    const int numColumns = PQnfields(result);
    const int numRows = PQntuples(result);

    int col, row, i;

    for (col = 0; col < numColumns; col++) {
        printf("| %-48s ", PQfname(result, col));
    }
    printf("|\n");

    printf("|");
    for (col = 0; col < numColumns; col++) {
        for (i = 0; i < 50; i++) {
            printf("-");
        }
        printf("|");
    }
    printf("\n");

    for (row = 0; row < numRows; row++) {
        for (col = 0; col < numColumns; col++) {
            printf("| %-48s ", PQgetvalue(result, row, col));
        }
        printf("|\n");
    }

    printf("\n");
}