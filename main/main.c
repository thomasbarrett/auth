#include <sha256.h>
#include <util.h>
#include <auth_client.h>
#include <cli.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include <regex.h>        
#include <stdbool.h>

#include <smtp.h>

char* create_jwt(const char *payload) {
    const char *header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    const char *header_enc = base64_url_encode(header, strlen(header), false);
    const char *payload_enc = base64_url_encode(payload, strlen(payload), false);
    char token[strlen(header_enc) + strlen(payload_enc) + 2];
    sprintf(token, "%s.%s", header_enc, payload_enc);
    uint8_t sig[32];
    hmac_sha256_sign(token, strlen(token), "test", 4, sig);
    const char *sig_enc = base64_url_encode(sig, 32, false);
    char *res = malloc(strlen(header_enc) + strlen(payload_enc) + strlen(sig_enc) + 3);
    sprintf(res, "%s.%s.%s", header_enc, payload_enc, sig_enc);
    free(header_enc);
    free(payload_enc);
    free(sig_enc);
    return res;
}

int create_password(PGconn *conn, password_t *pass) {
    const char *paramValues[] = {
        (const char *) pass->user_id,
        (const char *) pass->salt,
        (const char *) pass->hash
    };

    int paramLengths[] = {16, 32, 32};
    int paramFormats[] = {1, 1, 1};

    const char *cmd = "INSERT INTO passwords (user_id, salt, hash) \
                       VALUES ($1::uuid, $2::bytea, $3::bytea)";

    PGresult *res = PQexecParams(conn, cmd, 3, NULL, paramValues, paramLengths, paramFormats, 1);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        PQclear(res);
        return -1;
    }

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


int random_alnum(char *res, size_t len) {
    uint8_t bytes[len];
    FILE *urandom = fopen("/dev/urandom", "r");
    if (urandom == NULL) return -1;
    size_t nread = fread(bytes, 1, len, urandom);
    fclose(urandom);
    if (nread < len) return -1;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = bytes[i] % 62;
        if (byte < 26) res[i] = 'a' + byte;
        else if (byte < 52) res[i] = 'A' + byte - 26;
        else res[i] = '0' + byte - 52;
    }
    return 0;
}
/*
int signup(PGconn *conn, user_t *user) {
    uint8_t pass[17] = {0};
    random_alnum(pass, 16);
    printf("%s\n", pass);

    password_t password;
    int err = generate_salt(password.salt);
    if (err != 0) return -1;
    compute_password_hash(pass, password.salt, password.hash);
    
    err = create_user(conn, user);
    if (err != 0) return -1;

    memcpy(password.user_id, user->id, sizeof(user->id));
    err = create_password(conn, &password);
    if (err != 0) return -1;
}*/
/*
int reset_password(PGconn *conn, char *email) {
    user_t user;
    int err = get_user_with_email(conn, email, &user);
    if (err != 0) return -1;

    uint8_t one_time_pass[17] = {0};
    random_alnum(one_time_pass, 16);
    printf("%s\n", one_time_pass);

    password_t password;
    memcpy(password.user_id, user.id, sizeof(user.id));
    err = generate_salt(password.salt);
    if (err != 0) return -1;

    compute_password_hash(one_time_pass, password.salt, password.hash);
    err = update_password(conn, &password);
    if (err != 0) return -1;

    return 0;
}
*/
/*
int change_password(PGconn *conn, char *email, char *pass) {
    user_t user;
    int err = get_user_with_email(conn, email, &user);
    if (err != 0) return -1;

    password_t password;
    memcpy(password.user_id, user.id, sizeof(user.id));
    err = generate_salt(password.salt);
    if (err != 0) return -1;

    compute_password_hash(pass, password.salt, password.hash);
    err = update_password(conn, &password);
    if (err != 0) return -1;

    return 0;
}*/
/*
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
}*/

void user_print(user_t *user) {
    char uuid_string[37] = {0};
    uuid_to_string(user->id, uuid_string);
    printf("\033[3mid:\033[0m %s\n", uuid_string);
    printf("\033[3memail:\033[0m %s\n", user->email);
    printf("\033[3mfirst_name:\033[0m %s\n", user->first_name);
    printf("\033[3mlast_name:\033[0m %s\n", user->last_name);
}

int parse_uint32(char *val, void *res) {
    char *end_ptr = NULL;
    uint32_t x = strtoul(val, &end_ptr, 10);
    if (end_ptr != val + strlen(val)) return -1;
    *((uint32_t*) res) = x;
    return 0;
}

int parse_email(char *str, void *res) {
    if (strlen(str) > 63) return -1;
    strcpy((char *) res, str);
    return 0;
}

int parse_name(char *str, void *res) {
    if (strlen(str) > 15) return -1;
    strcpy((char *) res, str);
    return 0;
}

int parse_password(char *str, void *res) {
    if (strlen(str) > 31) return -1;
    strcpy((char *) res, str);
    return 0;
}

void list_users(command_t *self, auth_client_t *client, int argc, char *argv[]) {
    uint32_t page = 0;
    uint32_t page_size = 32;

    flag_t flags[] = {
        {"page", &page, parse_uint32, "index of result page"},
        {"page-size", &page_size, parse_uint32, "number of results per page"}
    };

    int err = flags_parse(flags, sizeof(flags), argc, argv);
    if (err != 0) {
        command_print_usage(self, NULL, 0, flags, sizeof(flags));
        return;
    }

    uint32_t offset = page_size * page;
    uint32_t limit = page_size;

    
    size_t n_users;
    user_t users[limit];
    err = auth_client_list_users(client, offset, limit, users, &n_users);
    if (err != 0) {
        fprintf(stderr, "error: unable to list users\n");
        return;
    }

    printf("%36s | %32s | %15s | %15s\n", "id", "email", "first_name", "last_name");
    for (size_t i = 0; i < n_users; i++) {
        char uuid_string[37] = {0};
        uuid_to_string(users[i].id, uuid_string);
        printf("%36s | %32s | %15s | %15s\n", uuid_string, users[i].email, users[i].first_name, users[i].last_name);
    }

}

void create_user(command_t *self, auth_client_t *client, int argc, char *argv[]) {
    user_t user;
    argument_t args[] = {
        {"email", user.email, parse_email, "user email"},
        {"first_name", user.first_name, parse_name, "user first name"},
        {"last_name", user.last_name, parse_name, "user last name"}
    };

    int err = argument_parse(args, sizeof(args), argc, argv);
    if (err != 0) {
        command_print_usage(self, args, sizeof(args), NULL, 0);
        return;
    }

    err = auth_client_create_user(client, &user);
    if (err != 0) {
        fprintf(stderr, "error: unable to create user\n");
        return;
    }

    printf("1 User Created:\n");
    user_print(&user);
}


void get_user(command_t *self, auth_client_t *client, int argc, char *argv[]) {
    user_t user;

    flag_t flags[] = {
        {"id", user.id, (parser_func_t) uuid_from_string, "user uuid"},
        {"email", user.email, parse_email, "user email"}
    };

    int err = flags_parse(flags, sizeof(flags), argc, argv);
    if (err != 0) {
        command_print_usage(self, NULL, 0, flags, sizeof(flags));
        return;
    }
    
    if (!(flags[0].found ^ flags[1].found)) {
        command_print_usage(self, NULL, 0, flags, sizeof(flags));
        return;
    }

    err = auth_client_get_user(client, &user);
    if (err != 0) {
        fprintf(stderr, "error: unable to get user\n");
        return;
    }

    printf("1 User Found:\n");
    user_print(&user);
}


void update_user(command_t *self, auth_client_t *client, int argc, char *argv[]) {
    user_t user;
    argument_t args[] = {
        {"id", user.id, (parser_func_t) uuid_from_string, "user uuid"},
        {"email", user.email, parse_email, "updated email"},
        {"first_name", user.first_name, parse_name, "updated first name"},
        {"last_name", user.last_name, parse_name, "updated last name"}
    };

    int err = argument_parse(args, sizeof(args), argc, argv);
    if (err != 0) {
        command_print_usage(self, args, sizeof(args), NULL, 0);
        return;
    }

    err = auth_client_update_user(client, &user);
    if (err != 0) {
        fprintf(stderr, "error: unable to update user\n");
        return;
    }
    
    printf("1 User Updated:\n");
    user_print(&user);
}

void delete_user(command_t *self, auth_client_t *client, int argc, char *argv[]) {
    user_t user;
    argument_t args[] = {
        {"id", user.id, (parser_func_t) uuid_from_string, "user uuid"},
    };

    int err = argument_parse(args, sizeof(args), argc, argv);
    if (err != 0) {
        command_print_usage(self, args, sizeof(args), NULL, 0);
        return;
    }

    err = auth_client_delete_user(client, &user);
    if (err != 0) {
        fprintf(stderr, "error: unable to get user\n");
        return;
    }

    printf("1 User Deleted:\n");
    user_print(&user);
}

void send_email(command_t *self, void *ctx, int argc, char *argv[]) {
    char *smtp_user = getenv("SMTP_USER");
    char *smtp_pass = getenv("SMTP_PASS"); 
    
    smtp_client_t *client = smtp_client_create("smtp.caltech.edu", 587, smtp_user, smtp_pass);
    if (client == NULL) {
        fprintf(stderr, "error: unable to connect to ESMTP server\n");
        return;
    }

    smtp_client_send(client, "ugcs@caltech.edu", argv[0], argv[1]);
}

void reset_pass(command_t *self, auth_client_t *client, int argc, char *argv[]) {
    user_t user = {0};
    argument_t args[] = {
        {"id", user.id, (parser_func_t) uuid_from_string, "user uuid"},
    };

    int err = argument_parse(args, sizeof(args), argc, argv);
    if (err != 0) {
        command_print_usage(self, args, sizeof(args), NULL, 0);
        return;
    }
    
    err = auth_client_get_user(client, &user);
    if (err != 0) {
        fprintf(stderr, "error: unable to get user\n");
        return;
    }

    uint8_t one_time_pass[17] = {0};
    random_alnum(one_time_pass, 16);

    password_t password;
    memcpy(password.user_id, user.id, sizeof(user.id));
    err = generate_salt(password.salt);
    if (err != 0) {
        printf("error: unable to generate salt\n");
        return;
    }

    compute_password_hash(one_time_pass, password.salt, password.hash);
    err = auth_client_update_password(client, &password);
    if (err != 0) {
        printf("error: unable to update password\n");
        return;
    }
    
    char *smtp_user = getenv("SMTP_USER");
    char *smtp_pass = getenv("SMTP_PASS"); 
    smtp_client_t *smtp_client = smtp_client_create("smtp.caltech.edu", 587, smtp_user, smtp_pass);
    if (client == NULL) {
        fprintf(stderr, "error: unable to connect to ESMTP server\n");
        return;
    }

    char email[4096];
    sprintf(email, "Subject: Password Reset\nHello %s,\n\n Your UGCS password has been reset. You may now use the following one-time password to login to your account:\n\n%s\n\nRegards,\nThe UGCS Team.", user.first_name, one_time_pass);
    smtp_client_send(smtp_client, "ugcs@caltech.edu", user.email, email);
}

void user_login(command_t *self, auth_client_t *client, int argc, char *argv[]) {
    user_t user = {0};
    char password[32] = {0};

    argument_t args[] = {
        {"email", user.email, (parser_func_t) parse_email, "user email"},
        {"password", password, (parser_func_t) parse_password, "user password"},
    };

    int err = argument_parse(args, sizeof(args), argc, argv);
    if (err != 0) {
        command_print_usage(self, args, sizeof(args), NULL, 0);
        return;
    }

    err = auth_client_get_user(client, &user);
    if (err != 0) {
        fprintf(stderr, "error: user '%s' does not exist\n", user.email);
        return;
    }

    password_t expected;
    memcpy(expected.user_id, user.id, sizeof(user.id));
    err = auth_client_get_password(client, &expected);
    if (err != 0) {
        fprintf(stderr, "error: user '%s' has no password set\n", user.email);
        return;
    }

    uint8_t hash[32];
    compute_password_hash(password, expected.salt, hash);
    if (memcmp(hash, expected.hash, sizeof(hash)) != 0) {
        fprintf(stderr, "error: invalid password for user '%s'\n", user.email);
        return;
    }

    char payload[1024];
    char user_id_str[37] = {0};
    uuid_to_string(user.id, user_id_str);
    sprintf(payload, "{\"user\":{\"id\":\"%s\",\"email\":\"%s\"}}", user_id_str, user.email);
    char *token = create_jwt(payload);
    printf("%s\n", token);
    free(token);
}

int main(int argc, char *argv[]) {

    auth_client_t *client = auth_client_create("localhost", 5432, "auth", "test", "test");
    if (client == NULL) {
        fprintf(stderr, "error: unable to connect to database\n");
        return 1;
    }
    
    command_t cmds[] = {
        {"user-list",   (command_func_t) list_users,  "list all existing users"},
        {"user-create", (command_func_t) create_user, "create a new user"},
        {"user-get",    (command_func_t) get_user,    "get a new user"},
        {"user-update", (command_func_t) update_user, "update an existing user"},
        {"user-delete", (command_func_t) delete_user, "delete an existing user"},
        {"user-login",  (command_func_t) user_login,  "attempt to login as the given user"},
        {"user-email",  (command_func_t) send_email,  "send an email to the given user"},
        {"pass-reset",  (command_func_t) reset_pass,  "reset the password of a given user"}
    };

    command_run(cmds, sizeof(cmds), client, argc, argv);
    
    auth_client_destroy(client);

    /*

    user_t user;
    err = login(conn, "tbarrett@caltech.edu", "CyfzcEFUYCuKqL5L", &user);
    if (err != 0) {
        printf("error: unable to login\n");
        PQfinish(conn);
        exit(1);
    }

    printf("login successful\n");


    user_t thomas;
    strcpy(thomas.email, "tbarrett@caltech.edu");
    strcpy(thomas.first_name, "thomas");
    strcpy(thomas.last_name, "barrett");
    err = signup(conn, &thomas);

    err = reset_password(conn, "admin@caltech.edu");
    if (err != 0) {
        printf("error: unable to reset password\n");
        PQfinish(conn);
        exit(1);
    }

    PQfinish(conn);

    char *smtp_user = getenv("SMTP_USER");
    char *smtp_pass = getenv("SMTP_PASS"); 
    smtp_client_t *client = smtp_client_create("smtp.caltech.edu", 587, smtp_user, smtp_pass);
    if (client == NULL) {
        printf("error: unable to connect to ESMTP server\n");
        return 1;
    }

    char *msg = "Subject: Test\r\nThis is a test.";
    smtp_client_send(client, "ugcs@caltech.edu", "tbarrett@caltech.edu", msg);
    return 0;
    */
   
}
