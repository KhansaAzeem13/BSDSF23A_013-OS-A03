/* --- Paste into src/shell.c (replace existing shell loop) --- */

#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

static void reap_children(void) {
    int status;
    pid_t pid;
    // Non-blocking reap of any finished children
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // If you have a remove_job_by_pid() function, call it so jobs list updates
        #ifdef HAS_REMOVE_JOB_BY_PID
        remove_job_by_pid(pid);
        #endif
        // Optionally print notification:
        if (WIFEXITED(status)) {
            // printf("Reaped PID %d (exit %d)\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            // printf("Reaped PID %d (signal %d)\n", pid, WTERMSIG(status));
        }
    }
}

static char *trim_whitespace(char *s) {
    if (!s) return s;
    // left trim
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    // right trim
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

static void parse_args(char *cmd, char **args, int *argcount) {
    // Simple whitespace tokenizer (not handling quotes)
    *argcount = 0;
    char *tok = strtok(cmd, " \t");
    while (tok != NULL && *argcount < MAX_ARGS - 1) {
        args[(*argcount)++] = tok;
        tok = strtok(NULL, " \t");
    }
    args[*argcount] = NULL;
}

static void launch_command(char *cmdstr, int background) {
    // cmdstr is modifiable (we'll use strtok etc). Keep a copy for job text.
    char cmdcopy[MAX_CMDLEN];
    strncpy(cmdcopy, cmdstr, sizeof(cmdcopy)-1);
    cmdcopy[sizeof(cmdcopy)-1] = '\0';

    // If this command contains pipes or redirection and you already have
    // an implementation in execute_with_redirection_and_pipes(), call it here:
    // if (strchr(cmdstr, '|') || strchr(cmdstr, '>') || strchr(cmdstr, '<')) {
    //     execute_with_redirection_and_pipes(cmdstr);
    //     return;
    // }

    // parse arguments
    char *args[MAX_ARGS];
    int argc = 0;
    parse_args(cmdstr, args, &argc);
    if (argc == 0) return;  // nothing to run

    // handle builtins in this helper? (Prefer existing handle_builtin)
    extern int handle_builtin(char **args); // use existing builtin handler if present
    if (handle_builtin(args)) return;

    pid_t cpid = fork();
    if (cpid < 0) {
        perror("fork");
        return;
    }
    if (cpid == 0) {
        // Child
        // Reset signals to default (optional)
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        execvp(args[0], args);
        // If execvp fails:
        perror("execvp");
        _exit(127);
    } else {
        // Parent
        if (background) {
            // Don't wait — record job
            add_job(cpid, cmdcopy);
            printf("[Job %d] started (PID=%d): %s\n", /*jobid*/ 0, (int)cpid, cmdcopy);
            // Note: if add_job returns job id, you can print proper job number.
        } else {
            // Foreground — wait for it
            int status;
            if (waitpid(cpid, &status, 0) < 0) {
                perror("waitpid");
            }
        }
    }
}

void shell_loop(void) {
    char linebuf[MAX_CMDLEN];

    while (1) {
        // 1) Reap any finished background children to avoid zombies
        reap_children();

        // 2) Prompt
        printf("%s", PROMPT); fflush(stdout);
        if (!fgets(linebuf, sizeof(linebuf), stdin)) {
            // EOF (Ctrl+D)
            printf("\n");
            break;
        }

        // Remove trailing newline
        linebuf[strcspn(linebuf, "\n")] = 0;
        char *line = trim_whitespace(linebuf);
        if (line[0] == '\0') continue;

        // 3) Split by ';' to get chained commands
        char *saveptr = NULL;
        char *command = strtok_r(line, ";", &saveptr);
        while (command != NULL) {
            char *cmd = trim_whitespace(command);
            if (cmd[0] != '\0') {
                // 4) Check for background execution: trailing '&'
                int background = 0;
                size_t len = strlen(cmd);
                if (len > 0 && cmd[len - 1] == '&') {
                    background = 1;
                    cmd[len - 1] = '\0';     // remove '&'
                    cmd = trim_whitespace(cmd); // trim again
                }

                // 5) Launch the command (foreground or background)
                launch_command(cmd, background);
            }

            // next command
            command = strtok_r(NULL, ";", &saveptr);
        }
    } // end while
}
