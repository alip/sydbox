/* Test program for t51-allow-proc-pid.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2010 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    char proc[32];

    snprintf(proc, 32, "/proc/%i", getpid());
    open(proc, O_RDWR);
    return (errno == EPERM) ? EXIT_FAILURE : EXIT_SUCCESS;
}
