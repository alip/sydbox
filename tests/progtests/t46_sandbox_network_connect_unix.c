/* Check program for t46-sandbox-network.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2010 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char **argv)
{
    int fd, len;
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

    if (connect(fd, (struct sockaddr *)&cli, sizeof(cli)) < 0) {
        perror("connect");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return EXIT_SUCCESS;
}
