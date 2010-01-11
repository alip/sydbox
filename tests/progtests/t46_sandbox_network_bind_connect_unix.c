/* Check program for t46-sandbox-network.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2010 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    pid_t pid;
    int fd, status;
    socklen_t len;
    struct sockaddr_un addr;

    if (argc < 2)
        return EXIT_FAILURE;

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, argv[1]);
    len = strlen(addr.sun_path) + sizeof(addr.sun_family);

    if ((pid = fork()) < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }
    else if (0 == pid) { /* child */
        sleep(1);

        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            perror("socket");
            return EXIT_FAILURE;
        }

        if (connect(fd, (struct sockaddr *)&addr, len) < 0) {
            perror("connect");
            close(fd);
            return EXIT_FAILURE;
        }
        close(fd);
        return EXIT_SUCCESS;
    }
    /* parent */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    if (bind(fd, (struct sockaddr *)&addr, len) < 0) {
        perror("bind");
        close(fd);
        kill(pid, SIGTERM);
        return EXIT_FAILURE;
    }

    if (listen(fd, 1) < 0) {
        perror("listen");
        close(fd);
        kill(pid, SIGTERM);
        return EXIT_FAILURE;
    }

    len = sizeof(addr);
    if ((fd = accept(fd, (struct sockaddr *)&addr, &len)) < 0) {
        perror("accept");
        close(fd);
        kill(pid, SIGTERM);
        return EXIT_FAILURE;
    }

    wait(&status);
    close(fd);
    return WEXITSTATUS(status);
}
