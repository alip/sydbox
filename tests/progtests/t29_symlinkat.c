/* Check program for t29-symlinkat.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#define _ATFILE_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    DIR *dot = opendir(".");
    if (NULL == dot)
        return EXIT_FAILURE;
    int dfd = dirfd(dot);
    if (-1 == dfd)
        return EXIT_FAILURE;

    if (0 > symlinkat("see.emily.play/gnome", dfd, "jugband.blues"))
        return (EPERM == errno) ? EXIT_FAILURE : EXIT_SUCCESS;
    else
        return EXIT_SUCCESS;
}
