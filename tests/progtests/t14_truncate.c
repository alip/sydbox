/* Check program for t14-truncate.bash
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

int main(void) {
    if (0 > truncate("arnold.layne", 0))
        return (EPERM == errno) ? EXIT_FAILURE : EXIT_SUCCESS;
    else
        return EXIT_SUCCESS;
}
