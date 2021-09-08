#include <sha256.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>

typedef struct user {
    uint8_t id[16];
    char email[64];
    char first_name[16];
    char last_name[16];
} user_t;

typedef struct password {
    uint8_t user_id[16];
    uint8_t salt[32];
    uint8_t hash[32];
} password_t;

void uuid_to_string(uint8_t *uuid, char *res) {
    sprintf(res, 
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid[0x0], uuid[0x1], uuid[0x2], uuid[0x3],
        uuid[0x4], uuid[0x5], uuid[0x6], uuid[0x7],
        uuid[0x8], uuid[0x9], uuid[0xa], uuid[0xb],
        uuid[0xc], uuid[0xd], uuid[0xe], uuid[0xf]
    );
}

int uuid_from_string(char *str, uint8_t *res) {
    int tmp[16] = {0};
    int err = sscanf(str, 
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        &tmp[0x0], &tmp[0x1], &tmp[0x2], &tmp[0x3],
        &tmp[0x4], &tmp[0x5], &tmp[0x6], &tmp[0x7],
        &tmp[0x8], &tmp[0x9], &tmp[0xa], &tmp[0xb],
        &tmp[0xc], &tmp[0xd], &tmp[0xe], &tmp[0xf]
    );
    if (err != 16) return -1;
    for (size_t i = 0; i < 16; i++) {
        res[i] = (uint8_t)(tmp[i]);
    }
    return 0;
}

void list_users(PGconn *conn, int offset, int limit) {
    const char *paramValues[2];
    int paramLengths[2];
    int paramFormats[2];

    uint32_t offsetParam = htonl(offset);
    paramValues[0] = &offsetParam;
    paramLengths[0] = sizeof(offsetParam);
    paramFormats[0] = 1;

    uint32_t limitParam = htonl(limit);
    paramValues[1] = &limitParam;
    paramLengths[1] = sizeof(limitParam);
    paramFormats[1] = 1;

    const char *command = "SELECT id, email, first_name, last_name FROM users OFFSET $1::int4 LIMIT $2::int4";
    PGresult *res = PQexecParams(conn, command, 2, NULL, paramValues, paramLengths, paramFormats, 1);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return;
    }
    
    printf("%-36s | %-24s | %-16s | %-16s\n", "uuid", "email", "first_name", "last_name");
    for (int i = 0; i < PQntuples(res); i++) {
        uint8_t *uuid = PQgetvalue(res, i, 0);
        char uuid_string[37];
        uuid_to_string(uuid, uuid_string);
        char *email = PQgetvalue(res, i, 1);
        char *first_name = PQgetvalue(res, i, 2);
        char *last_name = PQgetvalue(res, i, 3);
        printf("%-36s | %-24s | %-16s | %-16s\n", uuid_string, email, first_name, last_name);
    }

    PQclear(res);
}

int get_user(PGconn *conn, uint8_t *id, user_t *user) {
    const char *paramValues[1];
    int paramLengths[1];
    int paramFormats[1];

    paramValues[0] = id;
    paramLengths[0] = 16;
    paramFormats[0] = 1;

    const char *command = "SELECT email, first_name, last_name FROM users WHERE id = $1::uuid";
    PGresult *res = PQexecParams(conn, command, 1, NULL, paramValues, paramLengths, paramFormats, 1);
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

    memcpy(user->id, id, 16);
    strcpy(user->email, PQgetvalue(res, 0, 0));
    strcpy(user->first_name, PQgetvalue(res, 0, 1));
    strcpy(user->last_name, PQgetvalue(res, 0, 2));
    PQclear(res);
    return 0;
}

int get_user_with_email(PGconn *conn, char *email, user_t *user) {
    const char *paramValues[1];
    int paramLengths[1];
    int paramFormats[1];

    paramValues[0] = email;
    paramLengths[0] = strlen(email);
    paramFormats[0] = 0;

    const char *command = "SELECT id, first_name, last_name FROM users WHERE email = $1";
    PGresult *res = PQexecParams(conn, command, 1, NULL, paramValues, paramLengths, paramFormats, 1);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return -1;
    }
  
    size_t id_len = PQgetlength(res, 0, 0);
    size_t first_name_len = PQgetlength(res, 0, 1);
    size_t last_name_len = PQgetlength(res, 0, 2);
    if (id_len != 16 || first_name_len > 16 || last_name_len > 16) {
        PQclear(res);
        return -1;
    }

    strcpy(user->email, email);
    memcpy(user->id, PQgetvalue(res, 0, 0), 16);
    strcpy(user->first_name, PQgetvalue(res, 0, 1));
    strcpy(user->last_name, PQgetvalue(res, 0, 2));
    PQclear(res);
    return 0;
}

int get_password(PGconn *conn, uint8_t *user_id, password_t *password) {
    const char *paramValues[1];
    int paramLengths[1];
    int paramFormats[1];

    paramValues[0] = user_id;
    paramLengths[0] = 16;
    paramFormats[0] = 1;

    const char *command = "SELECT salt, hash FROM passwords WHERE user_id = $1::uuid";
    PGresult *res = PQexecParams(conn, command, 1, NULL, paramValues, paramLengths, paramFormats, 1);
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

    memcpy(password->user_id, user_id, 16);
    memcpy(password->salt, PQgetvalue(res, 0, 0), 32);
    memcpy(password->hash, PQgetvalue(res, 0, 1), 32);
    PQclear(res);
    return 0;
}

int generate_salt(uint8_t *salt) {
    FILE *urandom = fopen("/dev/urandom", "r");
    if (urandom == NULL) return -1;
    size_t nread = fread(salt, 1, 32, urandom);
    fclose(urandom);
    if (nread < 32) return -1;
    return 0;
}

void compute_password_hash(char *password, uint8_t *salt, uint8_t *res) {
    size_t password_len = strlen(password);
    uint8_t concat[password_len + 32];
    memcpy(concat, password, password_len);
    memcpy(concat + password_len, salt, 32);
    sha256(concat, password_len + 32, res);
}

int signup(PGconn *conn, user_t *user, char *pass) {
    password_t password;
    int err = generate_salt(password.salt);
    if (err != 0) return -1;
    compute_password_hash(pass, password.salt, password.hash);
    
    err = create_user(conn, user);
    if (err != 0) return -1;

    err = create_password(conn, password);
    if (err != 0) return -1;
}

int login(PGconn *conn, char *email, char *password, user_t *res) {
    user_t user;
    int err = get_user_with_email(conn, email, &user);
    if (err != 0) return -1;

    password_t expected;
    err = get_password(conn, user.id, &expected);
    if (err != 0) return -1;

    uint8_t hash[32];
    compute_password_hash(password, expected.salt, hash);
    if (memcmp(hash, expected.hash, sizeof(hash)) != 0) return -1;
    *res = user;
    return 0;
}

int main(void) {
    PGconn *conn = PQconnectdb("host=localhost user=test password=test dbname=auth");
    if (PQstatus(conn) == CONNECTION_BAD) {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }

    uint8_t uuid[16];
    int err = uuid_from_string("be468d79-0317-427e-885c-56433c3c55bc", uuid);
    if (err != 0) {
        PQfinish(conn);
        exit(1);
    }

    user_t user;
    err = login(conn, "admin@caltech.edu", "admin", &user);
    if (err != 0) {
        PQfinish(conn);
        exit(1);
    }

    PQfinish(conn);
    return 0;
}