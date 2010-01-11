/* Check program for t09-access.bash
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

enum test {
    T_READ,
    T_WRITE,
    T_EXEC
};

int main(int argc, char **argv) {
    int mode;
    int t = atoi(argv[1]);

    if (T_READ == t)
        mode = R_OK;
    else if (T_WRITE == t)
        mode = W_OK;
    else if (T_EXEC == t)
        mode = X_OK;
    else
        abort();

    if (0 > access("arnold.layne", mode))
        return (EPERM == errno) ? EXIT_FAILURE : EXIT_SUCCESS;
    else
        return EXIT_SUCCESS;
}
