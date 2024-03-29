#include <errno.h>
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

int web_ui_test_get(int client_socket, HttpRequest *request, int conn_index) {
    int retval = 0;

    unsigned int i;
    unsigned int j;
    unsigned int k;

    UiTestResult ui_test_result;
    if (core_ui_test(&ui_test_result, client_socket, conn_index) == -1) {
        retval = -1;
        goto clean_data;
    }

    /** TODO: improve http response headers */
    char response_headers[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "\r\n";

    char *template;
    if (read_file_from_path_relative_to_project_root(&template, "src/web/pages/ui_test/ui-test.html") == -1) {
        free(template);
        template = NULL;
        return -1;
    }

    char *response;
    response = (char *)malloc((strlen(response_headers) + strlen(template)) * (sizeof *response) + 1);
    if (response == NULL) {
        fprintf(stderr, "Failed to allocate memory for response\nError code: %d\n", errno);
        return -1;
    }

    if (sprintf(response, "%s%s", response_headers, template) < 0) {
        fprintf(stderr, "Failed to construct response\nError code: %d\n", errno);
        free(template);
        template = NULL;
        free(response);
        response = NULL;
        return -1;
    }

    free(template);
    template = NULL;

    /**
     * u_t_ stands for users_table_
     */
    char ***u_t_column_names = NULL;
    unsigned short u_t_columns = 4;
    unsigned short u_t_column_cells = 1;
    u_t_column_names = web_utils_matrix_2d_allocation(u_t_column_names, u_t_columns, u_t_column_cells);
    if (u_t_column_names == NULL) {
        fprintf(stderr, "Failed to allocate memory for u_t_column_names\nError code: %d\n", errno);
        retval = -1;
        goto clean_response_buffer;
    }

    for (i = 0; i < u_t_columns; i++) {
        for (j = 0; j < u_t_column_cells; j++) {
            u_t_column_names[i][j] = ui_test_result.users_data.columns[i];
        }
    }

    if (te_multiple_substring_swap("{{ for->users_table_column_names }}", "{{ end for->users_table_column_names }}", u_t_column_cells, u_t_column_names, &response, u_t_columns) == -1) {
        retval = -1;
        goto clean_u_t_column_names;
    }

    unsigned int amount_of_users = ui_test_result.users_data.rows;

    if (te_multiple_substring_swap("{{ for->users_rows }}", "{{ end for->users_rows }}", 0, NULL, &response, amount_of_users) == -1) {
        retval = -1;
        goto clean_u_t_column_names;
    }

    char ****users;
    users = malloc(amount_of_users * (sizeof ***users));
    if (users == NULL) {
        retval = -1;
        goto clean_u_t_column_names;
    }

    for (i = 0; i < amount_of_users; ++i) {
        users[i] = NULL;
    }

    unsigned int amount_of_users_to_clean_up = amount_of_users;
    for (i = 0; i < amount_of_users; ++i) {
        users[i] = web_utils_matrix_2d_allocation(users[i], u_t_columns, u_t_column_cells);
        if (users[i] == NULL) {
            fprintf(stderr, "Failed to allocate memory for users[%d]\nError code: %d\n", i, errno);
            amount_of_users_to_clean_up = i + 1;
            retval = -1;
            goto clean_u_t_users;
        }
    }

    unsigned int amount_of_users_with_values_to_clean_up = amount_of_users;
    for (i = 0; i < amount_of_users; ++i) {
        size_t id_length = strlen(ui_test_result.users_data.users[i].id);
        size_t full_name_length = strlen(ui_test_result.users_data.users[i].full_name);
        size_t email_length = strlen(ui_test_result.users_data.users[i].email);
        size_t country_length = strlen(ui_test_result.users_data.users[i].country);

        users[i][0][0] = NULL;
        users[i][1][0] = NULL;
        users[i][2][0] = NULL;
        users[i][3][0] = NULL;

        users[i][0][0] = (char *)malloc(id_length * sizeof(char) + 1);
        users[i][1][0] = (char *)malloc(full_name_length * sizeof(char) + 1);
        users[i][2][0] = (char *)malloc(email_length * sizeof(char) + 1);
        users[i][3][0] = (char *)malloc(country_length * sizeof(char) + 1);

        if (users[i][0][0] == NULL || users[i][1][0] == NULL || users[i][2][0] == NULL || users[i][3][0] == NULL) {
            fprintf(stderr, "Failed to allocate memory for users[%d]\nError code: %d\n", i, errno);
            amount_of_users_with_values_to_clean_up = i + 1;
            retval = -1;
            goto clean_u_t_users_values;
        }
    }

    for (i = 0; i < amount_of_users; ++i) {
        size_t id_length = strlen(ui_test_result.users_data.users[i].id);
        if (memcpy(users[i][0][0], ui_test_result.users_data.users[i].id, id_length) == NULL) {
            fprintf(stderr, "Failed to copy id into memory buffer\nError code: %d\n", errno);
            retval = -1;
            goto clean_u_t_users_values;
        }
        users[i][0][0][id_length] = '\0';

        size_t full_name_length = strlen(ui_test_result.users_data.users[i].full_name);
        if (memcpy(users[i][1][0], ui_test_result.users_data.users[i].full_name, full_name_length) == NULL) {
            fprintf(stderr, "Failed to copy full_name into memory buffer\nError code: %d\n", errno);
            retval = -1;
            goto clean_u_t_users_values;
        }
        users[i][1][0][full_name_length] = '\0';

        size_t email_length = strlen(ui_test_result.users_data.users[i].email);
        if (memcpy(users[i][2][0], ui_test_result.users_data.users[i].email, email_length) == NULL) {
            fprintf(stderr, "Failed to copy email into memory buffer\nError code: %d\n", errno);
            retval = -1;
            goto clean_u_t_users_values;
        }
        users[i][2][0][email_length] = '\0';

        size_t country_length = strlen(ui_test_result.users_data.users[i].country);
        if (memcpy(users[i][3][0], ui_test_result.users_data.users[i].country, country_length) == NULL) {
            fprintf(stderr, "Failed to copy country into memory buffer\nError code: %d\n", errno);
            retval = -1;
            goto clean_u_t_users_values;
        }
        users[i][3][0][country_length] = '\0';

        if (te_multiple_substring_swap("{{ for->user_row_values }}", "{{ end for->user_row_values }}", 1, users[i], &response, 4) == -1) {
            retval = -1;
            goto clean_u_t_users_values;
        }
    }

    unsigned int amount_of_countries = ui_test_result.countries_data.rows;
    if (te_multiple_substring_swap("{{ for->countries_rows }}", "{{ end for->countries_rows }}", 0, NULL, &response, amount_of_countries) == -1) {
        retval = -1;
        goto clean_u_t_users_values;
    }

    unsigned short c_t_columns = 1;
    unsigned short c_t_column_cells = 1;

    char ****countries;
    countries = malloc(amount_of_countries * (sizeof ***countries));
    if (countries == NULL) {
        retval = -1;
        goto clean_u_t_users_values;
    }

    for (i = 0; i < amount_of_countries; ++i) {
        countries[i] = NULL;
    }

    unsigned int amount_of_countries_to_clean_up = amount_of_countries;
    for (i = 0; i < amount_of_countries; ++i) {
        countries[i] = web_utils_matrix_2d_allocation(countries[i], c_t_columns, c_t_column_cells);
        if (countries[i] == NULL) {
            fprintf(stderr, "Failed to allocate memory for countries[%d]\nError code: %d\n", i, errno);
            amount_of_countries_to_clean_up = i + 1;
            retval = -1;
            goto clean_c_t_countries;
        }
    }

    unsigned int amount_of_countries_with_values_to_clean_up = amount_of_countries;
    for (i = 0; i < amount_of_countries; ++i) {
        size_t country_name_length = strlen(ui_test_result.countries_data.countries[i].country_name);

        countries[i][0][0] = NULL;

        countries[i][0][0] = (char *)malloc(country_name_length * sizeof(char) + 1);

        if (countries[i][0][0] == NULL) {
            fprintf(stderr, "Failed to allocate memory for countries[%d]\nError code: %d\n", i, errno);
            amount_of_countries_with_values_to_clean_up = i + 1;
            retval = -1;
            goto clean_c_t_countries_values;
        }
    }

    for (i = 0; i < amount_of_countries; ++i) {
        size_t country_name_length = strlen(ui_test_result.countries_data.countries[i].country_name);
        if (memcpy(countries[i][0][0], ui_test_result.countries_data.countries[i].country_name, country_name_length) == NULL) {
            fprintf(stderr, "Failed to copy country_name into memory buffer\nError code: %d\n", errno);
            retval = -1;
            goto clean_c_t_countries_values;
        }
        countries[i][0][0][country_name_length] = '\0';

        if (te_multiple_substring_swap("{{ for->country_values }}", "{{ end for->country_values }}", 1, countries[i], &response, 1) == -1) {
            retval = -1;
            goto clean_c_t_countries_values;
        }
    }

    size_t post_rendering_response_length = strlen(response) * sizeof(char);

    if (send(client_socket, response, post_rendering_response_length, 0) == -1) {
        fprintf(stderr, "Failed send HTTP response\nError code: %d\n", errno);
        retval = -1;
        goto clean_c_t_countries_values;
    }

    close(client_socket);

clean_c_t_countries_values:
    for (i = 0; i < amount_of_countries_with_values_to_clean_up; ++i) {
        free(countries[i][0][0]);
        countries[i][0][0] = NULL;
    }

clean_c_t_countries:
    for (i = 0; i < amount_of_countries_to_clean_up; ++i) {
        web_utils_matrix_2d_free(countries[i], c_t_columns);
    }

    free(countries);
    countries = NULL;

clean_u_t_users_values:
    for (i = 0; i < amount_of_users_with_values_to_clean_up; ++i) {
        free(users[i][0][0]);
        users[i][0][0] = NULL;
        free(users[i][1][0]);
        users[i][1][0] = NULL;
        free(users[i][2][0]);
        users[i][2][0] = NULL;
        free(users[i][3][0]);
        users[i][3][0] = NULL;
    }

clean_u_t_users:
    for (i = 0; i < amount_of_users_to_clean_up; ++i) {
        web_utils_matrix_2d_free(users[i], u_t_columns);
    }

    free(users);
    users = NULL;

clean_u_t_column_names:
    web_utils_matrix_2d_free(u_t_column_names, u_t_columns);

clean_response_buffer:
    free(response);
    response = NULL;

clean_data:
    core_utils_ui_test_free(&ui_test_result);

    return retval;
}
