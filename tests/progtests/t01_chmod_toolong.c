/* Check program for t01-chmod.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009, 2010 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    char *long_dir, *fname;

    if (3 > argc)
        return EXIT_FAILURE;
    else {
        long_dir = argv[1];
        fname = argv[2];
    }

    for (int i = 0; i < DIR_COUNT; i++) {
        if (0 > chdir(long_dir)) {
            fprintf(stderr, "chdir(%i): %s\n", i, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    if (0 > chmod(fname, 0000)) {
        if (EPERM == errno)
            return EXIT_FAILURE;
        perror("chmod");
        return EXIT_SUCCESS;
    }
    else
        return EXIT_SUCCESS;
}
