#include <linux/limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "core/core.h"
#include "globals.h"
#include "template_engine/template_engine.h"
#include "utils/utils.h"
#include "web/web.h"

int web_page_ui_test_get(int client_socket, HttpRequest *request, int conn_index) {
    int i;
    int j;
    int k;

    UiTestView ui_test_view_data;
    if (core_view_ui_test(&ui_test_view_data, client_socket, conn_index) == -1) {
        return -1;
    }

    char response_headers[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "\r\n";

    char *response;
    if (web_utils_construct_response(&response, "src/web/pages/ui_test/ui-test.html", response_headers) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

    char ***users_table_column_names = NULL;
    int users_table_column_names_d1 = 4;
    int users_table_column_names_d2 = 1;
    users_table_column_names = web_utils_matrix_2d_allocation(users_table_column_names, users_table_column_names_d1, users_table_column_names_d2);
    if (users_table_column_names == NULL) {
        log_error("Failed to allocate memory for users_table_column_names\n");
        return -1;
    }

    for (i = 0; i < users_table_column_names_d1; i++) {
        for (j = 0; j < users_table_column_names_d2; j++) {
            users_table_column_names[i][j] = ui_test_view_data.users_data.columns[i];
        }
    }

    if (te_multiple_substring_swap("{{ for->users_table_column_names }}", "{{ end for->users_table_column_names }}", users_table_column_names_d2, users_table_column_names, &response, users_table_column_names_d1) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

    web_utils_matrix_2d_free(users_table_column_names, users_table_column_names_d1, users_table_column_names_d2);

    if (te_multiple_substring_swap("{{ for->users_rows }}", "{{ end for->users_rows }}", 0, NULL, &response, ui_test_view_data.users_data.rows) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

    for (i = 0; i < ui_test_view_data.users_data.rows; ++i) {
        char ***user = NULL;
        int user_d1 = 4;
        int user_d2 = 1;
        user = web_utils_matrix_2d_allocation(user, user_d1, user_d2);
        if (user == NULL) {
            log_error("Failed to allocate memory for user\n");
            return -1;
        }

        size_t id_length = strlen(ui_test_view_data.users_data.users[i].id);
        user[0][0] = (char *)malloc(id_length * sizeof(char) + 1);
        memcpy(user[0][0], ui_test_view_data.users_data.users[i].id, id_length);
        user[0][0][id_length] = '\0';

        size_t full_name_length = strlen(ui_test_view_data.users_data.users[i].full_name);
        user[1][0] = (char *)malloc(full_name_length * sizeof(char) + 1);
        memcpy(user[1][0], ui_test_view_data.users_data.users[i].full_name, full_name_length);
        user[1][0][full_name_length] = '\0';

        size_t email_length = strlen(ui_test_view_data.users_data.users[i].email);
        user[2][0] = (char *)malloc(email_length * sizeof(char) + 1);
        memcpy(user[2][0], ui_test_view_data.users_data.users[i].email, email_length);
        user[2][0][email_length] = '\0';

        size_t country_length = strlen(ui_test_view_data.users_data.users[i].country);
        user[3][0] = (char *)malloc(country_length * sizeof(char) + 1);
        memcpy(user[3][0], ui_test_view_data.users_data.users[i].country, country_length);
        user[3][0][country_length] = '\0';

        if (te_multiple_substring_swap("{{ for->user_row_values }}", "{{ end for->user_row_values }}", 1, user, &response, 4) == -1) {
            free(response);
            response = NULL;
            return -1;
        }

        for (k = 0; k < user_d1; k++) {
            free(user[k][0]);
            user[k][0] = NULL;
        }

        web_utils_matrix_2d_free(user, user_d1, user_d2);
    }

    if (te_multiple_substring_swap("{{ for->countries_rows }}", "{{ end for->countries_rows }}", 0, NULL, &response, ui_test_view_data.countries_data.rows) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

    for (i = 0; i < ui_test_view_data.countries_data.rows; ++i) {
        char ***country = NULL;
        int country_d1 = 1;
        int country_d2 = 1;
        country = web_utils_matrix_2d_allocation(country, country_d1, country_d2);
        if (country == NULL) {
            log_error("Failed to allocate memory for country\n");
            return -1;
        }

        size_t country_name_length = strlen(ui_test_view_data.countries_data.countries[i].country_name);
        country[0][0] = (char *)malloc(country_name_length * sizeof(char) + 1);
        memcpy(country[0][0], ui_test_view_data.countries_data.countries[i].country_name, country_name_length);
        country[0][0][country_name_length] = '\0';

        if (te_multiple_substring_swap("{{ for->country_values }}", "{{ end for->country_values }}", 1, country, &response, 1) == -1) {
            free(response);
            response = NULL;
            return -1;
        }

        for (k = 0; k < country_d1; k++) {
            free(country[k][0]);
            country[k][0] = NULL;
        }

        web_utils_matrix_2d_free(country, country_d1, country_d2);
    }

    core_utils_view_ui_test_free(&ui_test_view_data);

    size_t post_rendering_response_length = strlen(response) * sizeof(char);

    if (send(client_socket, response, post_rendering_response_length, 0) == -1) {
        log_error("Failed send HTTP response\n");
        free(response);
        response = NULL;
        return -1;
    }

    free(response);
    response = NULL;

    close(client_socket);
    return 0;
}
