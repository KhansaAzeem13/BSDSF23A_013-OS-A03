#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

/* Limits (adjust if needed) */
#define MAX_LEN 1024
#define MAXARGS 64
#define ARGLEN 128
#define PROMPT "FCIT> "

/* Prototypes of functions implemented elsewhere or in other modules */
int execute(char **args); /* execute external commands (in execute.c) */
int execute_with_redirection_and_pipes(char *cmdline); /* advanced exec */
int handle_builtin(char **args); /* returns 1 if handled, 0 otherwise */

/* Job control prototypes (defined in other file, keep as int for status) */
int add_job(pid_t pid, const char *cmd);
int print_jobs(void);
int bring_fg(int jobid);
int kill_job(int jobid);

/* This module's prototypes */
char *read_cmd(FILE *fp);
char **tokenize(const char *cmdline);
void free_token_list(char **list);
void shell_loop(void);
int parse_and_execute_if_block(const char *block);

#endif /* SHELL_H */
