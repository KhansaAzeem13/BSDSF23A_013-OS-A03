
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CMDLEN 1024
#define MAX_ARGS 64

// --- Job management prototypes (must exist for feature 6) ---
void add_job(pid_t pid, const char *cmd);
void remove_job_by_pid(pid_t pid);  // optional — only if implemented
int print_jobs(void);
int bring_fg(int jobid);
int kill_job(int jobid);

// --- Other features ---
int handle_builtin(char **args);
int execute_with_redirection_and_pipes(char *cmdline);
#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LEN 1024
#define MAX_TOKENS 100
#define MAX_HISTORY 50
#define MAX_JOBS 64
#define PROMPT "FCIT> "

typedef struct {
    pid_t pid;
    char cmd[MAX_LEN];
    int active;
} Job;

// Core shell functions
void shell_loop(void);
int handle_builtin(char **args);
int execute(char **args);
int execute_with_redirection_and_pipes(char *cmdline);

#endif
