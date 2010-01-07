/* Check program for t20-rename-first-atfdcwd.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009, 2010 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#define _ATFILE_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    if (0 > renameat(AT_FDCWD, "arnold.layne", AT_FDCWD, "lucifer.sam"))
        return (EPERM == errno) ? EXIT_FAILURE : EXIT_SUCCESS;
    else
        return EXIT_SUCCESS;
}
