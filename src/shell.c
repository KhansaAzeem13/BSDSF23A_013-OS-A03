#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 100
#define MAX_LINE 1024
#define PROMPT "FCIT> "

// ---------------- Variable System ----------------
static Var *var_head = NULL;

void set_variable(const char *name, const char *value) {
    Var *curr = var_head;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            strncpy(curr->value, value, sizeof(curr->value) - 1);
            curr->value[sizeof(curr->value) - 1] = '\0';
            return;
        }
        curr = curr->next;
    }
    Var *newvar = malloc(sizeof(Var));
    if (!newvar) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    strncpy(newvar->name, name, sizeof(newvar->name) - 1);
    newvar->name[sizeof(newvar->name) - 1] = '\0';
    strncpy(newvar->value, value, sizeof(newvar->value) - 1);
    newvar->value[sizeof(newvar->value) - 1] = '\0';
    newvar->next = var_head;
    var_head = newvar;
}

const char *get_variable(const char *name) {
    Var *curr = var_head;
    while (curr) {
        if (strcmp(curr->name, name) == 0)
            return curr->value;
        curr = curr->next;
    }
    return "";
}

void print_variables(void) {
    Var *curr = var_head;
    while (curr) {
        printf("%s=%s\n", curr->name, curr->value);
        curr = curr->next;
    }
}

// ---------------- Variable Helpers ----------------
int handle_assignment(char *cmd) {
    char *eq = strchr(cmd, '=');
    if (!eq) return 0; // Not an assignment

    *eq = '\0';
    char *name = cmd;
    char *value = eq + 1;

    // Ensure no spaces
    if (strchr(name, ' ') || strchr(value, ' '))
        return 0;

    set_variable(name, value);
    return 1;
}

void expand_variables(char **args) {
    for (int i = 0; args[i]; i++) {
        if (args[i][0] == '$') {
            const char *val = get_variable(args[i] + 1);
            char *expanded = malloc(strlen(val) + 1);
            if (!expanded) continue;
            strcpy(expanded, val);
            free(args[i]);
            args[i] = expanded;
        }
    }
}

// ---------------- Main Shell Loop ----------------
void shell_loop(void) {
    char line[MAX_LINE];

    while (1) {
        printf("%s", PROMPT);
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = '\0'; // remove newline

        if (strlen(line) == 0) continue;
        if (strcmp(line, "exit") == 0) break;

        // Handle variable assignment
        if (handle_assignment(line)) continue;

        // Tokenize input
        char *args[MAX_ARGS];
        char *tok = strtok(line, " ");
        int i = 0;
        while (tok && i < MAX_ARGS - 1) {
            args[i++] = strdup(tok);
            tok = strtok(NULL, " ");
        }
        args[i] = NULL;

        // Expand variables ($VAR)
        expand_variables(args);

        // Built-in command: set (list variables)
        if (args[0] && strcmp(args[0], "set") == 0) {
            print_variables();
            for (int j = 0; j < i; j++) free(args[j]);
            continue;
        }

        // Execute normal command
        execute(args);

        // Cleanup
        for (int j = 0; j < i; j++) free(args[j]);
    }
}
