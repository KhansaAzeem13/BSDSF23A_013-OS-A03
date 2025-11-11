/* src/execute.c
 * execute external commands; but check built-ins first to ensure
 * built-ins are handled even if the main loop doesn't call them.
 */

#include "shell.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* execute:
 * args is a NULL-terminated array of strings
 * returns 0 on success (or built-in handled), -1 on failure
 */
int execute(char **args) {
    if (args == NULL || args[0] == NULL) {
        return -1;
    }

    /* If this is a built-in, handle it here and return */
    if (handle_builtin(args)) {
        return 0;
    }

    pid_t cpid = fork();
    if (cpid < 0) {
        perror("fork");
        return -1;
    }

    if (cpid == 0) {
        /* Child: try to execute the external command */
        execvp(args[0], args);

        /* If execvp returns, it failed. Print a helpful message then exit. */
        fprintf(stderr, "Command not found: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    } else {
        /* Parent: wait for child to finish */
        int status = 0;
        if (waitpid(cpid, &status, 0) < 0) {
            perror("waitpid");
            return -1;
        }
    }

    return 0;
}
