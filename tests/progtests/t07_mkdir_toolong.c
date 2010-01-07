/* Check program for t07-mkdir.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
    char *long_dir, *dname;

    if (3 > argc)
        return EXIT_FAILURE;
    else {
        long_dir = argv[1];
        dname = argv[2];
    }

    for (int i = 0; i < DIR_COUNT; i++) {
        if (0 > chdir(long_dir))
            return EXIT_FAILURE;
    }

    if (0 > mkdir(dname, 0644))
        return (EPERM == errno) ? EXIT_FAILURE : EXIT_SUCCESS;
    else
        return EXIT_SUCCESS;
}
