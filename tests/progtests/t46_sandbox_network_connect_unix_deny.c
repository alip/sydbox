/* Check program for t46-sandbox-network.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2010 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char **argv)
{
    int fd, len, save_errno;
    struct sockaddr_un cli;

    if (argc < 2)
        return EXIT_FAILURE;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    cli.sun_family = AF_UNIX;
    strcpy(cli.sun_path, argv[1]);
    len = strlen(cli.sun_path) + sizeof(cli.sun_family);

    connect(fd, (struct sockaddr *)&cli, sizeof(cli));
    save_errno = errno;
    perror("connect");
    close(fd);
    return (save_errno == ECONNREFUSED) ? EXIT_FAILURE : EXIT_SUCCESS;
}
