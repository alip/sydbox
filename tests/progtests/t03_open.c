/* Check program for t03-open.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009, 2010 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

enum test {
    T_READONLY,
    T_WRONLY,
    T_RDWR,
};

int main(int argc, char **argv) {
    int fd;
    int t = atoi(argv[1]);
    char f[] = "arnold.layne";

    switch (t) {
        case T_READONLY:
            if (0 > open(f, O_RDONLY))
                return EXIT_FAILURE;
            else
                return EXIT_SUCCESS;
        case T_WRONLY:
            fd = open(f, O_WRONLY);
            if (0 > fd)
                return (EPERM == errno) ? EXIT_FAILURE : EXIT_SUCCESS;
            else {
                write(fd, "why can't you see?", 18);
                close(fd);
                return EXIT_SUCCESS;
            }
        case T_RDWR:
            fd = open(f, O_RDWR);
            if (0 > fd)
                return (EPERM == errno) ? EXIT_FAILURE : EXIT_SUCCESS;
            else {
                write(fd, "why can't you see?", 18);
                close(fd);
                return EXIT_SUCCESS;
            }

    }
    return EXIT_FAILURE;
}
