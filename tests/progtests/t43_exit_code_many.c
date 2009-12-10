/* Check program for t43-exit-code.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int parent_exit_code;
    int child_exit_code;
    int interval;
    pid_t child_pid;

    if (4 > argc)
        return EXIT_FAILURE;

    parent_exit_code = atoi(argv[1]);
    child_exit_code = atoi(argv[2]);
    interval = atoi(argv[3]);

    child_pid = fork();
    if (0 > child_pid)
        return EXIT_FAILURE;
    else if (0 == child_pid) {
        sleep(interval);
        return child_exit_code;
    }
    else
        return parent_exit_code;
}
