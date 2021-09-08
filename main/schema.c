#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>

int schema_version(PGconn *conn) {
    PGresult *res = PQexec(conn, "SELECT version from constants WHERE _ = true;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return 0;
    }
    
    if (PQntuples(res) < 1) {
        PQclear(res);
        return 0;
    }

    return atoi(PQgetvalue(res, 0, 0));
}

size_t get_file_size(FILE *file) {
    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    return size;
}

char* read_file(const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) return NULL;
    size_t size = get_file_size(file);

    char *command = calloc(size + 1, 1);
    if (command == NULL) {
        fclose(file);
        return NULL;
    }

    size_t nread = fread(command, 1, size, file);
    fclose(file);
    if (nread != size) {
        free(command);
        return NULL;
    }

    return command;
}

int main() {
    PGconn *conn = PQconnectdb("host=localhost user=test password=test dbname=auth");
    if (PQstatus(conn) == CONNECTION_BAD) {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }

    int version = schema_version(conn);
    printf("current version: %d\n", version);

    if (version == 0) {
        char *command = read_file("schema/version-1.sql");
        if (command == NULL) {
            printf("error: unable to read '%s'\n", "schema/version-1.sql");
            PQfinish(conn);
            exit(1);
        }

        PGresult *res = PQexec(conn, command);
        free(command);

        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            printf("error: %s\n", PQerrorMessage(conn));
            PQfinish(conn);
            exit(1);
        }
        
        PQclear(res);
    }
    
    PQfinish(conn);
    return 0;
}
