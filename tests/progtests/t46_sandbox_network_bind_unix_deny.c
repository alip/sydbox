/* Check program for t46-sandbox-network.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2010 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

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
    int fd, len;
    struct sockaddr_un srv;

    if (argc < 2)
        return EXIT_FAILURE;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    srv.sun_family = AF_UNIX;
    strcpy(srv.sun_path, argv[1]);
    len = strlen(srv.sun_path) + sizeof(srv.sun_family);

    if (bind(fd, (struct sockaddr *)&srv, len) < 0) {
        perror("bind");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return (errno == EADDRNOTAVAIL) ? EXIT_FAILURE : EXIT_SUCCESS;
}
