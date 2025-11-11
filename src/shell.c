#include "shell.h"

/*
 * read_cmd: reads either a single-line command or a multi-line if...then...fi block.
 * Returns a malloc'd string (caller must free). Returns NULL on EOF (Ctrl+D).
 */
char *read_cmd(FILE *fp) {
    char *buf = malloc(MAX_LEN);
    if (!buf) return NULL;
    buf[0] = '\0';

    /* Read first line */
    if (!fgets(buf, MAX_LEN, fp)) {
        free(buf);
        return NULL; /* EOF */
    }
    /* remove trailing newline */
    buf[strcspn(buf, "\n")] = 0;

    /* If it's an "if" line, continue reading until matching "fi" */
    char *trim = buf;
    while (*trim == ' ' || *trim == '\t') trim++;

    if (strncmp(trim, "if ", 3) == 0 || strcmp(trim, "if") == 0) {
        /* we're in an if...then...fi block */
        size_t used = strlen(buf);
        /* append newline to mark line boundary when parsing later */
        if (used + 2 < MAX_LEN) {
            buf[used++] = '\n';
            buf[used] = '\0';
        }

        char line[MAX_LEN];
        int found_fi = 0;
        while (fgets(line, sizeof(line), fp)) {
            /* strip newline */
            line[strcspn(line, "\n")] = 0;
            size_t linelen = strlen(line);
            if (used + linelen + 2 >= MAX_LEN) {
                /* need more space: reallocate */
                size_t newsize = MAX_LEN * 2;
                char *nbuf = realloc(buf, newsize);
                if (!nbuf) break;
                buf = nbuf;
                /* update MAX_LEN locally? we rely on allocated size only */
            }
            /* append line + newline */
            strcat(buf, line);
            strcat(buf, "\n");
            used = strlen(buf);

            /* check for fi on its own (with optional spaces/tabs) */
            char tmptrim[MAX_LEN];
            strncpy(tmptrim, line, sizeof(tmptrim)-1);
            tmptrim[sizeof(tmptrim)-1] = '\0';
            char *t = tmptrim;
            while (*t == ' ' || *t == '\t') t++;
            if (strcmp(t, "fi") == 0) {
                found_fi = 1;
                break;
            }
        }
        /* return entire block (includes final 'fi\n') */
        return buf;
    }

    /* normal single-line command */
    return buf;
}

/* tokenize: simple whitespace tokenization, returns NULL-terminated list.
   caller must free via free_token_list(). */
char **tokenize(const char *cmdline) {
    if (cmdline == NULL) return NULL;

    char *copy = strdup(cmdline);
    if (!copy) return NULL;

    char **args = malloc(sizeof(char*) * (MAXARGS + 1));
    if (!args) { free(copy); return NULL; }

    int i = 0;
    char *tok = strtok(copy, " \t");
    while (tok != NULL && i < MAXARGS) {
        args[i++] = strdup(tok);
        tok = strtok(NULL, " \t");
    }
    args[i] = NULL;
    free(copy);
    return args;
}

void free_token_list(char **list) {
    if (!list) return;
    for (int i = 0; list[i] != NULL; ++i) {
        free(list[i]);
    }
    free(list);
}

/* parse_and_execute_if_block:
   block is the full multiline string containing:
     if <cmd>
     then
       <commands...>
     [else
       <commands...>]
     fi
   Returns 0 on success (parsed & executed), -1 on parse error.
*/
int parse_and_execute_if_block(const char *block) {
    if (!block) return -1;

    /* Duplicate and split by lines */
    char *copy = strdup(block);
    if (!copy) return -1;

    char *lines[256];
    int nlines = 0;
    char *line = strtok(copy, "\n");
    while (line && nlines < 256) {
        /* trim leading/trailing spaces */
        while (*line == ' ' || *line == '\t') line++;
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == ' ' || line[len-1] == '\t')) {
            line[len-1] = '\0';
            len--;
        }
        lines[nlines++] = line;
        line = strtok(NULL, "\n");
    }

    if (nlines == 0) {
        free(copy);
        return -1;
    }

    /* Expect first line to start with 'if' */
    if (strncmp(lines[0], "if", 2) != 0) {
        free(copy);
        return -1;
    }

    /* parse the if command (after 'if') */
    const char *ifcmd = lines[0];
    while (*ifcmd == ' ' || *ifcmd == '\t') ifcmd++;
    if (strncmp(ifcmd, "if", 2) == 0) {
        ifcmd += 2;
        while (*ifcmd == ' ' || *ifcmd == '\t') ifcmd++;
    }

    /* find indices of then, else (if present), fi */
    int then_idx = -1, else_idx = -1, fi_idx = -1;
    for (int i = 1; i < nlines; ++i) {
        if (strcmp(lines[i], "then") == 0 && then_idx == -1) then_idx = i;
        else if (strcmp(lines[i], "else") == 0 && else_idx == -1) else_idx = i;
        else if (strcmp(lines[i], "fi") == 0) { fi_idx = i; break; }
    }

    if (then_idx == -1 || fi_idx == -1 || !(then_idx < fi_idx)) {
        free(copy);
        return -1;
    }

    /* gather 'then' commands (between then_idx+1 and else_idx-1 or fi_idx-1) */
    int then_start = then_idx + 1;
    int then_end = (else_idx == -1) ? fi_idx - 1 : else_idx - 1;

    /* gather 'else' commands if present */
    int else_start = -1, else_end = -1;
    if (else_idx != -1) {
        else_start = else_idx + 1;
        else_end = fi_idx - 1;
    }

    /* Execute the command in ifcmd: tokenize and fork/wait to gather exit status */
    char **ifargs = tokenize(ifcmd);
    if (!ifargs) { free(copy); return -1; }

    pid_t cpid = fork();
    if (cpid < 0) {
        perror("fork");
        free_token_list(ifargs);
        free(copy);
        return -1;
    } else if (cpid == 0) {
        /* child: use execute() function where possible or execvp fallback */
        if (handle_builtin(ifargs)) {
            /* builtin executed in child (rare) */
            exit(0);
        } else {
            /* attempt execvp directly */
            execvp(ifargs[0], ifargs);
            /* if execvp fails, print and exit with non-zero */
            fprintf(stderr, "execvp: %s: No such file or directory\n", ifargs[0]);
            exit(127);
        }
    } else {
        int status = 0;
        waitpid(cpid, &status, 0);
        int exitcode = WIFEXITED(status) ? WEXITSTATUS(status) : 1;

        /* choose which block to run */
        int run_then = (exitcode == 0);

        if (run_then) {
            for (int i = then_start; i <= then_end; ++i) {
                if (lines[i] == NULL) continue;
                if (strlen(lines[i]) == 0) continue;
                /* For each line, handle builtins or execute */
                char **args = tokenize(lines[i]);
                if (!args) continue;
                if (!handle_builtin(args)) {
                    /* check for pipes/redirection */
                    if (strchr(lines[i], '|') || strchr(lines[i], '>') || strchr(lines[i], '<')) {
                        execute_with_redirection_and_pipes(lines[i]);
                    } else {
                        execute(args);
                    }
                }
                free_token_list(args);
            }
        } else {
            if (else_idx != -1) {
                for (int i = else_start; i <= else_end; ++i) {
                    if (lines[i] == NULL) continue;
                    if (strlen(lines[i]) == 0) continue;
                    char **args = tokenize(lines[i]);
                    if (!args) continue;
                    if (!handle_builtin(args)) {
                        if (strchr(lines[i], '|') || strchr(lines[i], '>') || strchr(lines[i], '<')) {
                            execute_with_redirection_and_pipes(lines[i]);
                        } else {
                            execute(args);
                        }
                    }
                    free_token_list(args);
                }
            }
        }
    }

    free_token_list(ifargs);
    free(copy);
    return 0;
}

/* shell_loop: main loop: read commands, detect if-blocks, handle builtins and normal commands */
void shell_loop(void) {
    while (1) {
        printf("%s", PROMPT);
        fflush(stdout);

        char *cmdline = read_cmd(stdin);
        if (!cmdline) {
            /* EOF */
            printf("\n");
            break;
        }

        /* trim leading spaces */
        char *p = cmdline;
        while (*p == ' ' || *p == '\t') p++;

        if (strncmp(p, "if", 2) == 0) {
            /* handle multi-line if block */
            parse_and_execute_if_block(cmdline);
            free(cmdline);
            continue;
        }

        /* single-line command processing */
        /* simple check for empty */
        if (strlen(p) == 0) {
            free(cmdline);
            continue;
        }

        /* Tokenize */
        char **args = tokenize(p);
        if (!args) {
            free(cmdline);
            continue;
        }

        if (!handle_builtin(args)) {
            /* check for advanced syntax first (pipes/redirection) */
            if (strchr(p, '|') || strchr(p, '>') || strchr(p, '<')) {
                execute_with_redirection_and_pipes(p);
            } else {
                execute(args);
            }
        }

        free_token_list(args);
        free(cmdline);
    }
}

