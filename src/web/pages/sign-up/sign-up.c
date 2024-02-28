#include <argon2.h>
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
#include "web/web.h"

#define HASH_LENGTH 32
#define SALT_LENGTH 16
#define PASSWORD_BUFFER 255

int generate_salt(uint8_t *salt, size_t salt_size);

int web_page_sign_up_get(int client_socket, HttpRequest *request) {
    /** TODO: improve http response headers */
    char response_headers[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "\r\n";

    char *response;
    if (web_utils_construct_response(&response, "src/web/pages/sign-up/sign-up.html", response_headers) == -1) {
        free(response);
        response = NULL;
        return -1;
    }

    size_t post_rendering_response_length = strlen(response) * sizeof(char);

    if (send(client_socket, response, post_rendering_response_length, 0) == -1) {
        fprintf(stderr, "Failed send HTTP response\nError code: %d\n", errno);
        free(response);
        response = NULL;
        return -1;
    }

    free(response);
    response = NULL;

    return 0;
}

int web_page_sign_up_create_user_post(int client_socket, HttpRequest *request) {
    /**
     * This endpoint may return:
     * - (Error) HTML partials to display error messages (invalid inputs, server errors...).
     * - (Success) Redirect instructions in the headers
     */

    char response_headers[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "\r\n";

    web_utils_url_decode(&request->body);

    char *email = NULL;
    web_utils_parse_value(&email, "email", request->body);

    char *password = NULL;
    web_utils_parse_value(&password, "password", request->body);

    char *repeat_password = NULL;
    web_utils_parse_value(&repeat_password, "repeat_password", request->body);

    char *remember = NULL;
    web_utils_parse_value(&remember, "remember", request->body);

    uint8_t salt[SALT_LENGTH];
    memset(salt, 0, SALT_LENGTH);

    if (generate_salt(salt, SALT_LENGTH) == -1) {
        return -1;
    }

    uint32_t t_cost = 2;         /** 2-pass computation */
    uint32_t m_cost = (1 << 16); /** 64 mebibytes memory usage */
    uint32_t parallelism = 1;    /** number of threads and lanes */

    uint8_t hash[HASH_LENGTH];
    memset(hash, 0, sizeof(hash));

    char secure_password[PASSWORD_BUFFER];
    if (argon2i_hash_raw(t_cost, m_cost, parallelism, password, strlen(password), salt, SALT_LENGTH, hash, HASH_LENGTH) != ARGON2_OK) {
        fprintf(stderr, "Failed to create hash from password\nError code: %d\n", errno);
        return -1;
    }

    if (argon2i_hash_encoded(t_cost, m_cost, parallelism, password, strlen(password), salt, SALT_LENGTH, HASH_LENGTH, secure_password, PASSWORD_BUFFER) != ARGON2_OK) {
        fprintf(stderr, "Failed to encode hash\nError code: %d\n", errno);
        return -1;
    }

    if (argon2i_verify(secure_password, password, strlen(password)) != ARGON2_OK) {
        fprintf(stderr, "Failed to verify password\nError code: %d\n", errno);
        return -1;
    }

    printf("%s\n", secure_password);

    /** - Validate user input */

    /** - Save new user to the database and create login session. */

    /** - If transaction error, create create error response */

    free(email);
    email = NULL;

    free(password);
    password = NULL;

    free(repeat_password);
    repeat_password = NULL;

    free(remember);
    remember = NULL;

    if (send(client_socket, response_headers, strlen(response_headers), 0) == -1) {
        fprintf(stderr, "Failed send HTTP response\nError code: %d\n", errno);
        return -1;
    }

    return 0;
}

int generate_salt(uint8_t *salt, size_t salt_size) {
    FILE *dev_urandom = fopen("/dev/urandom", "rb");
    if (dev_urandom == NULL) {
        fprintf(stderr, "Error opening /dev/urandom\nError code: %d\n", errno);
        return -1;
    }

    if (fread(salt, 1, salt_size, dev_urandom) != salt_size) {
        fprintf(stderr, "Error reading from /dev/urandom\nError code: %d\n", errno);
        fclose(dev_urandom);
        return -1;
    }

    fclose(dev_urandom);

    return 0;
}
