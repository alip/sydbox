/* Check program for t48-sandbox-network-bindzero.bash
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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    pid_t pid;
    int fd, newfd, status;
    int pfd[2];
    char strport[16];
    socklen_t len;
    struct sockaddr_in addr;

    if (argc < 2)
        return EXIT_FAILURE;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &(addr.sin_addr));
    addr.sin_port = 0;

    if (0 > pipe(pfd)) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    if ((pid = fork()) < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }
    else if (0 == pid) { /* child */
        close(pfd[1]);
        sleep(1);

        read(pfd[0], strport, 16);
        close(pfd[0]);
        addr.sin_port = atoi(strport);

        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket");
            return EXIT_FAILURE;
        }

        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("connect");
            close(fd);
            return EXIT_FAILURE;
        }
        close(fd);
        return EXIT_SUCCESS;
    }
    /* parent */
    close(pfd[0]);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        kill(pid, SIGTERM);
        return EXIT_FAILURE;
    }

    /* Duplicate fd */
    newfd = dup2(fd, fd + 3);
    if (newfd < 0) {
        perror("dup2");
        close(fd);
        kill(pid, SIGTERM);
        return EXIT_FAILURE;
    }

    len = sizeof(addr);
    if (getsockname(newfd, (struct sockaddr *)&addr, &len) < 0) {
        perror("getsockname");
        close(pfd[1]);
        close(newfd);
        kill(pid, SIGTERM);
        return EXIT_FAILURE;
    }

    /* Write file descriptor */
    snprintf(strport, 16, "%i", addr.sin_port);
    write(pfd[1], strport, 16);
    close(pfd[1]);

    if (listen(newfd, 1) < 0) {
        perror("listen");
        close(newfd);
        kill(pid, SIGTERM);
        return EXIT_FAILURE;
    }

    len = sizeof(addr);
    if ((fd = accept(newfd, (struct sockaddr *)&addr, &len)) < 0) {
        perror("accept");
        close(newfd);
        kill(pid, SIGTERM);
        return EXIT_FAILURE;
    }

    wait(&status);
    close(newfd);
    return WEXITSTATUS(status);
}
