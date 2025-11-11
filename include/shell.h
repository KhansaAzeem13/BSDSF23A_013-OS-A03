#ifndef SHELL_H
#define SHELL_H

/* Standard headers needed by many implementation files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

/* Limits (adjust if needed) */
#define MAX_LEN 1024
#define MAXARGS 64
#define ARGLEN 128
#define PROMPT "FCIT> "
#define MAX_JOBS 64
#define JOB_CMDLEN 256

/* =======================
   JOB CONTROL STRUCTURES
   ======================= */
typedef struct {
    pid_t pid;
    int active;              /* 1 = running, 0 = finished/removed */
    char cmd[JOB_CMDLEN];    /* stored command string */
} Job;

/* =======================
   EXECUTION + BUILTINS
   ======================= */
int execute(char **args); /* execute external commands, returns status/int */
int execute_with_redirection_and_pipes(char *cmdline);
int handle_builtin(char **args); /* returns 1 if builtin handled, 0 otherwise */

/* =======================
   JOB CONTROL FUNCTIONS
   ======================= */
void add_job(pid_t pid, const char *cmd); /* add a background job */
void print_jobs(void);                    /* print current background jobs */
int bring_fg(int jobid);                  /* bring job to foreground */
int kill_job(int jobid);                  /* kill a job */

/* =======================
   SHELL LOOP + IF-THEN
   ======================= */
char *read_cmd(FILE *fp);
char **tokenize(const char *cmdline);
void free_token_list(char **list);
void shell_loop(void);
int parse_and_execute_if_block(const char *block);

/* =======================
   VARIABLE SUPPORT (Feature 8)
   ======================= */
typedef struct Var {
    char name[64];
    char value[256];
    struct Var *next;
} Var;

void set_variable(const char *name, const char *value);
const char *get_variable(const char *name);
void print_variables(void);

#endif /* SHELL_H */

