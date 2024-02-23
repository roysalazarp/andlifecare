#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>

#include "core/core.h"

void core_utils_view_ui_test_free(UiTestView *ui_test_view_data_buffer) {
    free(ui_test_view_data_buffer->users_data.users);
    ui_test_view_data_buffer->users_data.users = NULL;
    free(ui_test_view_data_buffer->countries_data.countries);
    ui_test_view_data_buffer->countries_data.countries = NULL;
}

void core_utils_print_query_result(PGresult *result) {
    const int num_columns = PQnfields(result);
    const int num_rows = PQntuples(result);

    int col;
    int row;
    int i;

    for (col = 0; col < num_columns; col++) {
        printf("| %-48s ", PQfname(result, col));
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
            printf("| %-48s ", PQgetvalue(result, row, col));
        }
        printf("|\n");
    }

    printf("\n");
}