/* Check program for t45-sandbox-exec.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009, 2010 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    int save_errno;

    char *myargv[] = {"/bin/true", NULL};

    execvp(myargv[0], myargv);
    save_errno = errno;
    perror("execvp");
    return (EACCES == save_errno) ? EXIT_FAILURE : EXIT_SUCCESS;
}
