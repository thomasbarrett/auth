#include <cli.h>
#include <string.h>
#include <stdio.h>

int flag_key(char *flag, char *key, size_t n) {
    if (strlen(flag) < 5) return -1;
    if (flag[0] != '-' || flag[1] != '-') return -1;
    char *end = strchr(flag, '=');
    if (end == NULL) return -1;
    size_t key_len = (size_t)(end - flag) - 2;
    if (key_len > n) return -1;
    memcpy(key, flag + 2, key_len);
    return 0;
}

int flag_val(char *flag, char *val, size_t n) {
    if (strlen(flag) < 5) return -1;
    char *start = strchr(flag, '=') + 1;
    if (start == NULL) return -1;
    char *end = flag + strlen(flag);
    size_t val_len = (size_t)(end - start);
    if (val_len > n) return -1;
    memcpy(val, start, val_len);
    return 0;
}

int flags_parse(flag_t *flags, size_t flags_size, int argc, char *argv[]) {
    size_t n_flags = flags_size / sizeof(flag_t);
    for (size_t i = 0; i < n_flags; i++) {
        flags[i].found = 0;
    }

    for (size_t i = 0; i < argc; i++) {
        char *flag = argv[i];
        
        char key[65] = {0};
        int err = flag_key(flag, key, 64);
        if (err != 0) return -1;
        
        char val[65] = {0};
        err = flag_val(flag, val, 64);
        if (err != 0) return -1;

        for (size_t j = 0; j < n_flags; j++) {
            if (strcmp(flags[j].key, key) == 0) {
                err = flags[j].parser(val, flags[j].value); 
                if (err != 0) return -1;
                flags[j].found = 1;
                break;
            }
        }
    }

    return 0;
}

int argument_parse(argument_t *args, size_t args_size, int argc, char *argv[]) {
    size_t n_args = args_size / sizeof(argument_t);
    if (argc < n_args) return -1;
    for (size_t i = 0; i < n_args; i++) {
        int err = args[i].parser(argv[i], args[i].value);
        if (err != 0) return -1;
    }
    return 0;
}

void command_print_usage(command_t *cmd, argument_t *args, size_t args_size, flag_t *flags, size_t flags_size) {
    size_t n_args = args_size / sizeof(argument_t);
    size_t n_flags = flags_size / sizeof(flag_t);

    printf("\nUSAGE\n  %s ", cmd->name);
    for (size_t i = 0; i < n_args; i++) {
        printf("[%s] ", args[i].name);
    }
    printf("\n");
    if (n_args != 0) {
        printf("\nARGUMENTS\n");
        for (size_t i = 0; i < n_args; i++) {
            printf("  %-24s %s\n", args[i].name, args[i].description);
        }
    }
    if (n_flags != 0) {
        printf("\nFLAGS\n");
        for (size_t i = 0; i < n_flags; i++) {
            printf("  %-24s %s\n", flags[i].key, flags[i].description);
        }
    }
    printf("\n");
}


void command_run(command_t *cmds, size_t cmds_size, void *ctx, int argc, char *argv[]) {
    size_t n_cmds = cmds_size / sizeof(command_t);
    int handled = 0;

    if (argc >= 2) {
        char *cmd = argv[1];
        for (size_t i = 0; i < n_cmds; i++) {
            if (strcmp(cmd, cmds[i].name) == 0) {
                cmds[i].handler(&cmds[i], ctx, argc - 2, argv + 2);
                handled = 1;
            }
        }
    }

    if (!handled) {
        printf("\nUSAGE\n  %s [COMMAND]\n", argv[0]);
        printf("\nCOMMANDS\n");
        for (size_t i = 0; i < n_cmds; i++) {
            printf("  %-24s %s\n", cmds[i].name, cmds[i].description);
        }
        printf("\n");
    }
}
