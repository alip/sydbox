/* Check program for t42-magic-wrap_lstat.bash
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
#include <sys/types.h>

int main(int argc, char **argv)
{
    char *long_dir, *fname;

    if (3 > argc)
        return EXIT_FAILURE;
    else {
        long_dir = argv[1];
        fname = argv[2];
    }

    for (int i = 0; i < 64; i++) {
        if (0 > chdir(long_dir))
            return EXIT_FAILURE;
    }

    if (0 > truncate(fname, 0))
        return (ENAMETOOLONG == errno) ? EXIT_FAILURE : EXIT_SUCCESS;
    else
        return EXIT_SUCCESS;
}
