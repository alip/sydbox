/* Test program for t50-rmdir-dangling-symlink.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2010 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc < 1)
        return EXIT_FAILURE;

    rmdir(argv[1]);
    if (errno != ENOTDIR) {
        perror("rmdir");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
