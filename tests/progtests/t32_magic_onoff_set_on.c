/* Check program for t32-magic-onoff.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009, 2010 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void)
{
    struct stat buf;

    if (0 > stat("/dev/sydbox/on", &buf)) {
        perror("stat");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
