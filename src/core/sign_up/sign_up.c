#include <argon2.h>
#include <errno.h>
#include <libpq-fe.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/core.h"
#include "globals.h"
#include "template_engine/template_engine.h"
#include "utils/utils.h"

#define HASH_LENGTH 32
#define SALT_LENGTH 16
#define PASSWORD_BUFFER 255

int generate_salt(uint8_t *salt, size_t salt_size);

int core_sign_up_create_user(SignUpCreateUserResult *result, SignUpCreateUserInput *input, int client_socket, int conn_index) {
    PGconn *conn = conn_pool[conn_index];

    /** Validate input data */

    /** Hash user password */
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
    if (argon2i_hash_raw(t_cost, m_cost, parallelism, input->password, strlen(input->password), salt, SALT_LENGTH, hash, HASH_LENGTH) != ARGON2_OK) {
        fprintf(stderr, "Failed to create hash from password\nError code: %d\n", errno);
        return -1;
    }

    if (argon2i_hash_encoded(t_cost, m_cost, parallelism, input->password, strlen(input->password), salt, SALT_LENGTH, HASH_LENGTH, secure_password, PASSWORD_BUFFER) != ARGON2_OK) {
        fprintf(stderr, "Failed to encode hash\nError code: %d\n", errno);
        return -1;
    }

    if (argon2i_verify(secure_password, input->password, strlen(input->password)) != ARGON2_OK) {
        fprintf(stderr, "Failed to verify password\nError code: %d\n", errno);
        return -1;
    }

    const char *paramValues[1];
    paramValues[0] = input->email;

    /** Check whether user already exists */
    PGresult *found_user = PQexecParams(conn, "SELECT * FROM app.users WHERE email = $1", 1, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(found_user) != PGRES_TUPLES_OK) {
        fprintf(stderr, "%s\nError code: %d\n", PQerrorMessage(conn), errno);
        return -1;
    }

    core_utils_print_query_result(found_user);

    unsigned int rows = PQntuples(found_user);
    if (rows > 0) {
        printf("User '%s' is already found in the database\n", input->email);
        /** TODO: Probably return some kind of error message to UI */
    }

    printf("%s\n", input->email);
    printf("%s\n", input->password);
    printf("%s\n", input->repeat_password);

    printf("%s\n", secure_password);

    /** Insert new user in users table */
    /** Create new session for user */
    /** return result */

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
