cat > src/shell.c <<'EOF'
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Simple tokenizer that returns malloc'd argv (strings point into line) */
char **parse_input(char *line) {
    char **args = malloc(sizeof(char*) * (MAX_ARGS));
    if (!args) return NULL;
    int i = 0;
    char *tok = strtok(line, " \t\n");
    while (tok != NULL && i < MAX_ARGS - 1) {
        args[i++] = tok;
        tok = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
    return args;
}

/* Built-in handler: returns 1 if handled, 0 if not */
int handle_builtin(char **args) {
    if (!args || !args[0]) return 0;

    if (strcmp(args[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    if (strcmp(args[0], "cd") == 0) {
        if (!args[1]) {
            char *home = getenv("HOME");
            if (!home) home = "/";
            if (chdir(home) != 0) perror("cd");
        } else {
            if (chdir(args[1]) != 0) perror("cd");
        }
        return 1;
    }

    if (strcmp(args[0], "help") == 0) {
        printf("Built-in commands:\n  cd <dir>\n  help\n  exit\n  jobs\n  fg <jobid>\n  kill <jobid>\n  history\n");
        return 1;
    }

    if (strcmp(args[0], "history") == 0) {
        /* history implemented elsewhere (if at all) */
        return 1;
    }

    /* Job control builtins exist in execute.c (only declarations are here) */
    if (strcmp(args[0], "jobs") == 0) {
        print_jobs();
        return 1;
    }
    if (strcmp(args[0], "fg") == 0) {
        if (!args[1]) fprintf(stderr, "Usage: fg <jobid>\n");
        else bring_fg(atoi(args[1]));
        return 1;
    }
    if (strcmp(args[0], "kill") == 0) {
        if (!args[1]) fprintf(stderr, "Usage: kill <jobid>\n");
        else kill_job(atoi(args[1]));
        return 1;
    }

    return 0;
}

/* Main shell loop */
void shell_loop(void) {
    char line[MAX_LEN];

    while (1) {
        printf("%s", PROMPT);
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0') continue;

        /* if pipes or redirection present, hand the raw line to executor */
        if (strchr(line, '|') || strchr(line, '<') || strchr(line, '>')) {
            char *dup = strdup(line);
            if (!dup) continue;
            execute_with_redirection_and_pipes(dup);
            free(dup);
            continue;
        }

        /* normal path */
        char *dup = strdup(line);
        if (!dup) continue;
        char **args = parse_input(dup);
        if (!args) { free(dup); continue; }

        if (!handle_builtin(args)) {
            execute(args);
        }

        free(args);
        free(dup);
    }
}
EOF
