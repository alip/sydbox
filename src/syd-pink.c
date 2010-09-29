/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2010 Ali Polatel <alip@exherbo.org>
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

#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "syd-log.h"
#include "syd-pink.h"

/* Wrappers around pinktrace functions */

inline
bool pinkw_trace_setup_all(pid_t pid)
{
    return pink_trace_setup(pid, PINK_TRACE_OPTION_SYSGOOD
                | PINK_TRACE_OPTION_FORK
                | PINK_TRACE_OPTION_VFORK
                | PINK_TRACE_OPTION_CLONE
                | PINK_TRACE_OPTION_EXEC);
}

bool pinkw_encode_stat(pid_t pid, pink_bitness_t bitness)
{
    struct stat buf;

    memset(&buf, 0, sizeof(struct stat));
    buf.st_mode = S_IFCHR | (S_IRUSR | S_IWUSR) | (S_IRGRP | S_IWGRP) | (S_IROTH | S_IWOTH);
    buf.st_rdev = 259; // /dev/null
    buf.st_mtime = -842745600; // ;)

    return pink_encode_simple(pid, bitness, 1, &buf, sizeof(struct stat));
}

struct sydbox_addr *pinkw_get_socket_addr(pid_t pid, pink_bitness_t bitness, unsigned ind, long *fd)
{
    pink_socket_address_t addr;
    struct sydbox_addr *saddr;

    if (!pink_decode_socket_address(pid, bitness, ind, fd, &addr))
        return NULL;

    saddr = g_new0(struct sydbox_addr, 1);
    saddr->family = addr.family;
    switch (saddr->family) {
        case -1: /* Unknown family */
            return saddr;
        case AF_UNIX:
            saddr->u.saun.exact = true;
            saddr->u.saun.abstract = (addr.u.sa_un.sun_path[0] == '\0' && addr.u.sa_un.sun_path[1] != '\0');
            if (saddr->u.saun.abstract)
                strncpy(saddr->u.saun.sun_path, addr.u.sa_un.sun_path + 1, strlen(addr.u.sa_un.sun_path + 1) + 1);
            else
                strncpy(saddr->u.saun.sun_path, addr.u.sa_un.sun_path, strlen(addr.u.sa_un.sun_path) + 1);
            break;
        case AF_INET:
            saddr->u.sa.port[0] = ntohs(addr.u.sa_in.sin_port);
            saddr->u.sa.port[1] = saddr->u.sa.port[0];
            saddr->u.sa.netmask = 32;
            memcpy(&saddr->u.sa.sin_addr, &addr.u.sa_in.sin_addr, sizeof(struct in_addr));
            break;
#if PINKTRACE_HAVE_IPV6
        case AF_INET6:
            saddr->u.sa6.port[0] = ntohs(addr.u.sa6.sin6_port);
            saddr->u.sa6.port[1] = saddr->u.sa6.port[0];
            saddr->u.sa6.netmask = 64;
            memcpy(&saddr->u.sa6.sin6_addr, &addr.u.sa6.sin6_addr, sizeof(struct in6_addr));
            break;
#endif /* PINKTRACE_HAVE_IPV6 */
        default:
            /* nothing */
            ;
    }
    return saddr;
}

/* TODO: Consider moving this to pinktrace as well. */
char *pinkw_stringify_argv(pid_t pid, pink_bitness_t bitness, unsigned ind)
{
    int save_errno;
    char *s;
    const char *sep;
    long addr;
    union {
        unsigned int p32;
        unsigned long p64;
        char data[sizeof(long)];
    } cp;
    GString *res;

    if (!pink_util_get_arg(pid, bitness, ind, &addr)) {
        save_errno = errno;
        g_info("failed to get address of argument %d: %s", ind, g_strerror(errno));
        errno = save_errno;
        return NULL;
    }

    res = g_string_new("");
    cp.p64 = 1;
    for (sep = "";;sep = ", ") {
        if (!pink_util_moven(pid, addr, cp.data, (bitness == PINK_BITNESS_32) ? 4 : 8)) {
            g_string_append_printf(res, "%#lx", addr);
            return g_string_free(res, FALSE);
        }
        if (bitness == PINK_BITNESS_32)
            cp.p64 = cp.p32;
        if (cp.p64 == 0)
            break;

        g_string_append(res, sep);
        s = (char *)g_malloc(sizeof(char) * 256);
        s[255] = '\0';
        if (pink_util_moven(pid, cp.p64, s, 256)) {
            g_string_append_printf(res, "%#lx", cp.p64);
            g_free(s);
            break;
        }
        g_string_append_c(res, '"');
        g_string_append(res, s);
        g_string_append_c(res, '"');
        g_free(s);

        addr += (bitness == PINK_BITNESS_32) ? 4 : 8;
    }
    if (cp.p64)
        g_string_append_printf(res, "%s...", sep);
    return g_string_free(res, FALSE);
}
