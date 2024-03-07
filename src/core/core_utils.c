#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>

#include "core/core.h"

void core_utils_ui_test_free(UiTestResult *ui_test_data_buffer) {
    free(ui_test_data_buffer->users_data.users);
    ui_test_data_buffer->users_data.users = NULL;
    free(ui_test_data_buffer->countries_data.countries);
    ui_test_data_buffer->countries_data.countries = NULL;
}

void core_utils_print_query_result(PGresult *query_result) {
    const int num_columns = PQnfields(query_result);
    const int num_rows = PQntuples(query_result);

    int col;
    int row;
    int i;

    for (col = 0; col < num_columns; col++) {
        printf("| %-48s ", PQfname(query_result, col));
    }
    printf("|\n");

    printf("|");
    for (col = 0; col < num_columns; col++) {
        for (i = 0; i < 50; i++) {
            printf("-");
        }
        printf("|");
    }
    printf("\n");

    for (row = 0; row < num_rows; row++) {
        for (col = 0; col < num_columns; col++) {
            printf("| %-48s ", PQgetvalue(query_result, row, col));
        }
        printf("|\n");
    }

    printf("\n");
}