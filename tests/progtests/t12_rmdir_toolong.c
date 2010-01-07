/* Check program for t12-rmdir.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009, 2010 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    char *long_dir, *tmpdir;

    if (3 > argc)
        return EXIT_FAILURE;
    else {
        long_dir = argv[1];
        tmpdir = argv[2];
    }

    for (int i = 0; i < 64; i++) {
        if (0 > chdir(long_dir))
            return EXIT_FAILURE;
    }

    if (0 > rmdir(tmpdir))
        return (EPERM == errno) ? EXIT_FAILURE : EXIT_SUCCESS;
    else
        return EXIT_SUCCESS;
}
