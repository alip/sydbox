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
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char **argv)
{
    int fd, save_errno;
    struct sockaddr_in srv;

    if (argc < 3)
        return EXIT_FAILURE;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &(srv.sin_addr));
    srv.sin_port = htons(atoi(argv[2]));

    bind(fd, (struct sockaddr *)&srv, sizeof(srv));
    save_errno = errno;
    perror("bind");
    close(fd);
    return (save_errno == EADDRNOTAVAIL) ? EXIT_FAILURE : EXIT_SUCCESS;
}
