#include "shell.h"

/* ---------- READ COMMAND ---------- */
char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL; // Handle Ctrl+D
    }

    cmdline[pos] = '\0';
    return cmdline;
}

/* ---------- TOKENIZE COMMAND ---------- */
char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++; // skip spaces
        if (*cp == '\0') break;
        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) {
            len++;
        }
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    arglist[argnum] = NULL;
    return arglist;
}

/* ---------- HANDLE BUILT-IN COMMANDS ---------- */
int handle_builtin(char **args)
{
    if (args == NULL || args[0] == NULL)
        return 0;

    /* exit */
    if (strcmp(args[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    /* cd */
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        }
        return 1; // handled
    }

    /* help */
    if (strcmp(args[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <dir>  - change directory\n");
        printf("  help      - show this help message\n");
        printf("  exit      - exit the shell\n");
        printf("External commands like ls, pwd, whoami are also supported.\n");
        return 1;
    }

    return 0; // not a built-in
}

/* ---------- MAIN SHELL LOOP ---------- */
void shell_loop(void)
{
    char* cmdline;
    char** args;

    while (1) {
        cmdline = read_cmd(PROMPT, stdin);
        if (cmdline == NULL) {
            printf("\n");
            break;
        }

        args = tokenize(cmdline);
        if (args == NULL) {
            free(cmdline);
            continue;
        }

        /* check built-ins before executing external commands */
        if (handle_builtin(args)) {
            free(cmdline);
            for (int i = 0; args[i] != NULL; i++)
                free(args[i]);
            free(args);
            continue;
        }

        execute(args); // external command execution

        free(cmdline);
        for (int i = 0; args[i] != NULL; i++)
            free(args[i]);
        free(args);
    }
}
