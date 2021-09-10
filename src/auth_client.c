#include <auth_client.h>
#include <libpq-fe.h>
#include <string.h>
#include <stdlib.h>

typedef struct auth_client {
    PGconn *conn;
} auth_client_t;

auth_client_t* auth_client_create(char *host, int port, char *name, char *user, char *pass) {
    if (strlen(host) > 32 || strlen(name) > 32 || strlen(user) > 32 || strlen(pass) > 32) {
        return NULL;
    }
    char params[1024];
    sprintf(params, "host=%s user=%s password=%s dbname=%s", host, user, pass, name); 
    PGconn *conn = PQconnectdb(params);
    if (PQstatus(conn) == CONNECTION_BAD) {
        PQfinish(conn);
        return NULL;
    }

    auth_client_t *res = malloc(sizeof(auth_client_t));
    if (res == NULL) return NULL;
    res->conn = conn;
    return res;
}

void auth_client_destroy(auth_client_t *client) {
    PQfinish(client->conn);
    free(client);
}

int auth_client_create_user(auth_client_t *client, user_t *user) {
    const char *paramValues[] = {
        (const char *) user->email,
        (const char *) user->first_name,
        (const char *) user->last_name
    };

    int paramLengths[] = {
        strlen(user->email),
        strlen(user->first_name),
        strlen(user->last_name)
    };

    int paramFormats[] = {0, 0, 0};

    const char *cmd = "INSERT INTO users (email, first_name, last_name) \
                       VALUES ($1::email, $2::varchar, $3::varchar) \
                       RETURNING id";

    PGresult *res = PQexecParams(client->conn, cmd, 3, NULL, paramValues, paramLengths, paramFormats, 1);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return -1;
    }

    memcpy(user->id, PQgetvalue(res, 0, 0), 16);
    return 0;
}

int auth_client_get_user(auth_client_t *client, user_t *user) {
    const char *paramValues[1];
    int paramLengths[1];
    int paramFormats[1];
    const char *command;

    if (strlen(user->email) == 0) {
        paramValues[0] = (const char *) user->id;
        paramLengths[0] = 16;
        paramFormats[0] = 1;
        command = "SELECT id, email, first_name, last_name FROM users WHERE id = $1::uuid LIMIT 1";
    } else {
        paramValues[0] = (const char *) user->email;
        paramLengths[0] = strlen(user->email);
        paramFormats[0] = 0;
        command = "SELECT id, email, first_name, last_name FROM users WHERE email = $1::email LIMIT 1";
    }

    PGresult *res = PQexecParams(client->conn, command, 1, NULL, paramValues, paramLengths, paramFormats, 1);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return -1;
    }
  
    size_t email_len = PQgetlength(res, 0, 1);
    size_t first_name_len = PQgetlength(res, 0, 2);
    size_t last_name_len = PQgetlength(res, 0, 3);
    if (email_len > 64 || first_name_len > 16 || last_name_len > 16) {
        PQclear(res);
        return -1;
    }

    memcpy(user->id, PQgetvalue(res, 0, 0), 16);
    strcpy(user->email, PQgetvalue(res, 0, 1));
    strcpy(user->first_name, PQgetvalue(res, 0, 2));
    strcpy(user->last_name, PQgetvalue(res, 0, 3));
    PQclear(res);
    return 0;
}

int auth_client_update_user(auth_client_t *client, user_t *user) {
    const char *paramValues[] = {
        (const char *) user->id,
        (const char *) user->email,
        (const char *) user->first_name,
        (const char *) user->last_name
    };
    int paramLengths[] = {16, strlen(user->email), strlen(user->first_name), strlen(user->last_name)};
    int paramFormats[] = {1, 0, 0, 0};
    const char *cmd = "UPDATE users SET email = $2::email, first_name = $3::varchar, last_name = $4::varchar WHERE id = $1::uuid";
    PGresult *res = PQexecParams(client->conn, cmd, 4, NULL, paramValues, paramLengths, paramFormats, 1);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}

int auth_client_delete_user(auth_client_t *client, user_t *user) {
    const char *paramValues[] = {(const char *) user->id};
    int paramLengths[] = {16};
    int paramFormats[] = {1};

    const char *cmd = "DELETE FROM users WHERE id = $1::uuid RETURNING email, first_name, last_name";
    PGresult *res = PQexecParams(client->conn, cmd, 1, NULL, paramValues, paramLengths, paramFormats, 1);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return -1;
    }

    size_t email_len = PQgetlength(res, 0, 0);
    size_t first_name_len = PQgetlength(res, 0, 1);
    size_t last_name_len = PQgetlength(res, 0, 2);
    if (email_len > 64 || first_name_len > 16 || last_name_len > 16) {
        PQclear(res);
        return -1;
    }

    strcpy(user->email, PQgetvalue(res, 0, 0));
    strcpy(user->first_name, PQgetvalue(res, 0, 1));
    strcpy(user->last_name, PQgetvalue(res, 0, 2));
    PQclear(res);
    return 0;
}

int auth_client_list_users(auth_client_t *client, int offset, int limit, user_t *users, size_t *users_len) {
    const char *paramValues[2];
    int paramLengths[2];
    int paramFormats[2];

    uint32_t offsetParam = htonl(offset);
    paramValues[0] = (const char *) &offsetParam;
    paramLengths[0] = sizeof(offsetParam);
    paramFormats[0] = 1;

    uint32_t limitParam = htonl(limit);
    paramValues[1] = (const char *) &limitParam;
    paramLengths[1] = sizeof(limitParam);
    paramFormats[1] = 1;

    const char *command = "SELECT id, email, first_name, last_name FROM users OFFSET $1::int4 LIMIT $2::int4";
    PGresult *res = PQexecParams(client->conn, command, 2, NULL, paramValues, paramLengths, paramFormats, 1);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return -1;
    }
    
    *users_len = PQntuples(res);
    for (int i = 0; i < PQntuples(res); i++) {
        memcpy(users[i].id, PQgetvalue(res, i, 0), 16);
        strcpy(users[i].email, PQgetvalue(res, i, 1));
        strcpy(users[i].first_name, PQgetvalue(res, i, 2));
        strcpy(users[i].last_name, PQgetvalue(res, i, 3));
    }

    PQclear(res);
    return 0;
}

int auth_client_create_password(auth_client_t *client, password_t *password) {
    return 0;
}

int auth_client_get_password(auth_client_t *client, password_t *password) {
    const char *paramValues[1];
    int paramLengths[1];
    int paramFormats[1];

    paramValues[0] = (const char *) password->user_id;
    paramLengths[0] = 16;
    paramFormats[0] = 1;

    const char *command = "SELECT salt, hash FROM passwords WHERE user_id = $1::uuid";
    PGresult *res = PQexecParams(client->conn, command, 1, NULL, paramValues, paramLengths, paramFormats, 1);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return -1;
    }
  
    size_t salt_len = PQgetlength(res, 0, 0);
    size_t hash_len = PQgetlength(res, 0, 1);
    if (salt_len != 32 || hash_len != 32) {
        PQclear(res);
        return -1;
    }

    memcpy(password->salt, PQgetvalue(res, 0, 0), 32);
    memcpy(password->hash, PQgetvalue(res, 0, 1), 32);
    PQclear(res);
    return 0;
}

int auth_client_update_password(auth_client_t *client, password_t *password) {
     const char *paramValues[] = {
        (const char *) password->user_id, 
        (const char *) password->salt,
        (const char *) password->hash
    };
    int paramLengths[] = {16, 32, 32};
    int paramFormats[] = {1, 1, 1};

    const char *command = "UPDATE passwords SET salt = $2::bytea, hash = $3::bytea WHERE user_id = $1::uuid";
    PGresult *res = PQexecParams(client->conn, command, 3, NULL, paramValues, paramLengths, paramFormats, 1);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int auth_client_destroy_password(auth_client_t *client, password_t *password) {
    return 0;
}
