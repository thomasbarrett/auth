#ifndef CLI_H
#define CLI_H

#include <stdlib.h>
#include <stdbool.h>

typedef int (*parser_func_t)(char *, void *);

typedef struct argument {
    char *name;
    void *value;
    parser_func_t parser;
    char *description;
} argument_t;

typedef struct flag {
    char *key;
    void *value;
    parser_func_t parser;
    char *description;
    bool found;
} flag_t;

struct command;
typedef void (*command_func_t)(struct command *self, void *ctx, int argc, char *argv[]);

typedef struct command {
    char *name;
    command_func_t handler;
    char *description;
} command_t;


void command_run(command_t *cmds, size_t cmds_size, void *ctx, int argc, char *argv[]);
void command_print_usage(command_t *cmd, argument_t *args, size_t args_size, flag_t *flags, size_t flags_size);
int argument_parse(argument_t *args, size_t args_size, int argc, char *argv[]);
int flags_parse(flag_t *flags, size_t flags_size, int argc, char *argv[]);

#endif /* CLI_H */
