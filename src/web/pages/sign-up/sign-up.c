#include <argon2.h>
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

#define SALT_SIZE 16
#define HASH_SIZE 64

int verify_password(const char *password, const char *hash);
int hash_password(const char *password, char *hash, size_t hash_len);
void generate_salt(char *salt, size_t salt_size);

/* Reviewed: Fri 17. Feb 2024 */
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

int web_page_sign_up_create_user_post(int client_socket, HttpRequest *request) {
    /**
     * Probably at the beginning of this function create the headers for the response
     * for all kind of scenarios: 200 Success, 400 Bad Request, ...
     *
     * This function may return html to swap in the response (maybe with an error message) or
     * headers to redirect to the dashboard page if the signup was successful.
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

    printf("%s\n", email);
    printf("%s\n", password);
    printf("%s\n", repeat_password);

    /**
     * we probably don't need remember to signup or ever
     */
    printf("%s\n", remember);

    char hashed_password[HASH_SIZE];
    hash_password(password, hashed_password, sizeof(hashed_password));

    printf("%s\n", password);
    printf("%s\n", hashed_password);

    /**
     * store:
     * - email
     * - salt
     * - hashed_password
     */

    /**
     * Create valid session for user and attach it to response header
     * Add redirect to dashboard page and since response headers already send a valid session
     * the dashboard page will see that the user is logged in and return to the browser successfully
     */

    free(email);
    email = NULL;

    free(password);
    password = NULL;

    free(repeat_password);
    repeat_password = NULL;

    free(remember);
    remember = NULL;

    if (send(client_socket, response_headers, strlen(response_headers), 0) == -1) {
        log_error("Failed send HTTP response\n");
        return -1;
    }

    close(client_socket);
    return 0;

    return 0;
}

void generate_salt(char *salt, size_t salt_size) {
    FILE *dev_urandom = fopen("/dev/urandom", "rb");
    if (dev_urandom == NULL) {
        perror("Error opening /dev/urandom");
        exit(EXIT_FAILURE);
    }

    if (fread(salt, 1, salt_size, dev_urandom) != salt_size) {
        perror("Error reading from /dev/urandom");
        fclose(dev_urandom);
        exit(EXIT_FAILURE);
    }

    fclose(dev_urandom);
}

int hash_password(const char *password, char *hashed_password_buffer, size_t hashed_password_buffer_length) {
    char salt[SALT_SIZE];
    unsigned short i;

    for (i = 0; i < SALT_SIZE; i++) {
        salt[i] = 0;
    }

    generate_salt(salt, SALT_SIZE);

    unsigned int time_cost = 2;
    unsigned int memory_cost = 65536;
    unsigned int threads = 4;

    if (argon2_hash(time_cost, memory_cost, threads, password, strlen(password), salt, SALT_SIZE, hashed_password_buffer, hashed_password_buffer_length, NULL, 0, Argon2_i, ARGON2_VERSION_NUMBER) != ARGON2_OK) {
        log_error("Failed to hash password\n");
        return -1;
    }

    return 0;
}

int verify_password(const char *password, const char *hash) { return argon2_verify(hash, password, strlen(password), Argon2_i); }