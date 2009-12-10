/* Check program for t43-exit-code.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int exit_code;

    if (2 > argc)
        return EXIT_FAILURE;

    exit_code = atoi(argv[1]);
    return exit_code;
}
