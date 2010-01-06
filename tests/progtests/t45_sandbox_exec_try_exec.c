/* Check program for t45-sandbox-exec.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    char *myargv[] = {"/bin/true", NULL};

    execvp(myargv[0], myargv);
    perror("execvp");
    return (EACCES == errno) ? EXIT_FAILURE : EXIT_SUCCESS;
}
