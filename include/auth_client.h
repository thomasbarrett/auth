#ifndef AUTH_CLIENT_H
#define AUTH_CLIENT_H

#include <stdint.h>
#include <stdlib.h>

/**
 * auth_client_t wraps a client connection to the authentication database.
 */
typedef struct auth_client auth_client_t;

/**
 * user_t represents a user in the database.
 */
typedef struct user {
    uint8_t id[16];
    char email[64];
    char first_name[16];
    char last_name[16];
} user_t;

/**
 * password_t represents a password in the database.
 */
typedef struct password {
    uint8_t user_id[16];
    uint8_t salt[32];
    uint8_t hash[32];
} password_t;

/**
 * auth_client_create creates a new client connection to database with the
 * given hostname and port. 
 * 
 * @param host the database hostname.
 * @param port the database port.
 * @param name the database name.
 * @param user the database user.
 * @param pass the database password.
 */
auth_client_t* auth_client_create(char *host, int port, char *name, char *user, char *pass);

int auth_client_create_user(auth_client_t *client, user_t *user);
int auth_client_get_user(auth_client_t *client, user_t *user);
int auth_client_update_user(auth_client_t *client, user_t *user);
int auth_client_delete_user(auth_client_t *client, user_t *user);
int auth_client_list_users(auth_client_t *client, int offset, int limit, user_t *users, size_t *users_len);

int auth_client_create_password(auth_client_t *client, password_t *password);
int auth_client_get_password(auth_client_t *client, password_t *password);
int auth_client_update_password(auth_client_t *client, password_t *password);
int auth_client_delete_password(auth_client_t *client, password_t *password);

/**
 * auth_client_destroy destroys all resources used by the client.
 * @param client the client.
 */
void auth_client_destroy(auth_client_t *client);

#endif /* AUTH_CLIENT_H */
