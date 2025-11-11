#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

static Job jobs[MAX_JOBS];
static int job_count = 0;

// ---------------- Job Handling ----------------
void add_job(pid_t pid, const char *cmd) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        jobs[job_count].active = 1;
        strncpy(jobs[job_count].cmd, cmd, sizeof(jobs[job_count].cmd));
        printf("[Job %d] started (PID=%d): %s\n", job_count + 1, pid, cmd);
        job_count++;
    }
}

void print_jobs(void) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].active)
            printf("[%d] PID=%d running  %s\n", i + 1, jobs[i].pid, jobs[i].cmd);
    }
    }

int bring_fg(int jobid) {
    if (jobid < 1 || jobid > job_count || !jobs[jobid - 1].active) {
        fprintf(stderr, "Invalid job id\n");
        return -1;
    }
    pid_t pid = jobs[jobid - 1].pid;
    printf("Bringing job [%d] (PID=%d) to foreground...\n", jobid, pid);
    int status;
    waitpid(pid, &status, 0);
    jobs[jobid - 1].active = 0;
    return 0;
}

int kill_job(int jobid) {
    if (jobid < 1 || jobid > job_count || !jobs[jobid - 1].active) {
        fprintf(stderr, "Invalid job id\n");
        return -1;
    }
    pid_t pid = jobs[jobid - 1].pid;
    if (kill(pid, SIGKILL) == 0) {
        printf("[Job %d] killed (PID=%d): %s\n", jobid, pid, jobs[jobid - 1].cmd);
        jobs[jobid - 1].active = 0;
        return 0;
    } else {
        perror("kill");
        return -1;
    }
}

// ---------------- Execution (Normal + Background) ----------------
int execute(char **args) {
    if (args[0] == NULL)
        return 1;

    int background = 0;
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "&") == 0) {
            background = 1;
            args[i] = NULL;
            break;
        }
    }

    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        if (background) {
            char cmdline[256] = "";
            for (int i = 0; args[i] != NULL; i++) {
                strcat(cmdline, args[i]);
                strcat(cmdline, " ");
            }
            add_job(pid, cmdline);
        } else {
            int status;
            waitpid(pid, &status, 0);
        }
    } else {
        perror("fork");
    }
    return 1;
}

// ---------------- Redirection & Piping ----------------
int execute_with_redirection_and_pipes(char *cmdline) {
    char *commands[10];
    int num_cmds = 0;
    commands[num_cmds++] = strtok(cmdline, "|");

    while ((commands[num_cmds] = strtok(NULL, "|")) != NULL)
        num_cmds++;

    int pipefd[2], in_fd = 0;

    for (int i = 0; i < num_cmds; i++) {
        pipe(pipefd);
        pid_t pid = fork();

        if (pid == 0) {
            dup2(in_fd, 0);
            if (i < num_cmds - 1)
                dup2(pipefd[1], 1);

            close(pipefd[0]);
            close(pipefd[1]);

            // Handle redirection
            char *args[64];
            char *token = strtok(commands[i], " ");
            int j = 0;
            int fd;

            while (token != NULL) {
                if (strcmp(token, ">") == 0) {
                    token = strtok(NULL, " ");
                    fd = open(token, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                } else if (strcmp(token, ">>") == 0) {
                    token = strtok(NULL, " ");
                    fd = open(token, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                } else if (strcmp(token, "<") == 0) {
                    token = strtok(NULL, " ");
                    fd = open(token, O_RDONLY);
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                } else {
                    args[j++] = token;
                }
                token = strtok(NULL, " ");
            }
            args[j] = NULL;

            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            wait(NULL);
            close(pipefd[1]);
            in_fd = pipefd[0];
        }
    }

    return 1;
}

int handle_builtin(char **args) {
    if (args[0] == NULL)
        return 1;

    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } 
    else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    } 
    else if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            printf("%s\n", cwd);
        else
            perror("pwd");
        return 1;
    } 
    else if (strcmp(args[0], "jobs") == 0) {
        print_jobs();
        return 1;
    } 
    else if (strcmp(args[0], "fg") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "fg: missing job id\n");
        } else {
            bring_fg(atoi(args[1]));
        }
        return 1;
    } 
    else if (strcmp(args[0], "kill") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "kill: missing job id\n");
        } else {
            kill_job(atoi(args[1]));
        }
        return 1;
    }

    return 0; // Not a built-in command
}
