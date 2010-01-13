/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009, 2010 Ali Polatel <alip@exherbo.org>
 *
 * This file is part of the sydbox sandbox tool. sydbox is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * sydbox is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>

#include <glib.h>

#include "syd-proc.h"
#include "syd-wrappers.h"

char *pgetcwd(pid_t pid)
{
    int ret;
    char *cwd;
    char linkcwd[64];
    snprintf(linkcwd, 64, "/proc/%i/cwd", pid);

    // First try ereadlink()
    cwd = ereadlink(linkcwd);
    if (G_LIKELY(NULL != cwd))
        return cwd;
    else if (ENAMETOOLONG != errno)
        return NULL;

    // Now try egetcwd()
    errno = 0;
    ret = echdir(linkcwd);
    if (G_LIKELY(0 == ret))
        return egetcwd();
    errno = ENAMETOOLONG;
    return NULL;
}

char *pgetdir(pid_t pid, int dfd) {
    int ret;
    char *dir;
    char linkdir[128];
    snprintf(linkdir, 128, "/proc/%i/fd/%d", pid, dfd);

    // First try ereadlink()
    dir = ereadlink(linkdir);
    if (G_LIKELY(NULL != dir))
        return dir;
    else if (ENAMETOOLONG != errno) {
        if (ENOENT == errno) {
            /* The file descriptor doesn't exist!
             * Correct errno to EBADF.
             */
            errno = EBADF;
        }
        return NULL;
    }

    // Now try egetcwd()
    errno = 0;
    ret = echdir(linkdir);
    if (G_LIKELY(0 == ret))
        return egetcwd();
    errno = ENAMETOOLONG;
    return NULL;
}

static bool proc_lookup_inode(pid_t pid, int fd, unsigned *inode)
{
    char *bracket, *sock;
    char linkfd[256];

    snprintf(linkfd, 256, "/proc/%i/fd/%i", pid, fd);
    sock = ereadlink(linkfd);
    if (G_UNLIKELY(NULL == sock))
        return false;

    bracket = strchr(sock, '[');
    if (G_UNLIKELY(NULL == bracket)) {
        g_free(sock);
        return false;
    }

    *inode = atoi(++bracket);
    g_free(sock);
    return true;
}

static bool proc_lookup_port_internal(const char *path, unsigned myinode, unsigned *localport)
{
    unsigned inode;
    char buf[4096];
    FILE *pfd;

    pfd = fopen(path, "r");
    if (NULL == pfd)
        return false;

    while (fgets(buf, 4096, pfd) != NULL) {
        if (2 != sscanf(buf, "%*u: %*X:%x %*X:%*x %*x %*X:%*X %*x:%*X %*x %*u %*u %u",
                    localport, &inode))
            continue;
        if (inode == myinode) {
            fclose(pfd);
            return true;
        }
    }
    fclose(pfd);
    return false;
}

int proc_lookup_port(pid_t pid, int fd, int af)
{
    unsigned myinode, localport;

    if (!proc_lookup_inode(pid, fd, &myinode))
        return -1;

    if (af == AF_INET) {
        if (proc_lookup_port_internal("/proc/net/tcp", myinode, &localport))
            return localport;
        if (proc_lookup_port_internal("/proc/net/udp", myinode, &localport))
            return localport;
        return -1;
    }
#if HAVE_IPV6
    else if (af == AF_INET6) {
        if (proc_lookup_port_internal("/proc/net/tcp6", myinode, &localport))
            return localport;
        if (proc_lookup_port_internal("/proc/net/udp6", myinode, &localport))
            return localport;
        return -1;
    }
#endif /* HAVE_IPV6 */
    errno = EINVAL;
    return -1;
}

