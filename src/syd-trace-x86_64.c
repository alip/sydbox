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

#include <stdbool.h>

#include <string.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glib.h>

#include "syd-trace.h"
#include "syd-trace-util.h"

#define ORIG_ACCUM      (8 * ORIG_RAX)
#define ACCUM           (8 * RAX)
static const long syscall_args[2][MAX_ARGS] = {
    {8 * RBX, 8 * RCX, 8 * RDX, 8 * RSI, 8 * RDI, 8 * RBP},
    {8 * RDI, 8 * RSI, 8 * RDX, 8 * R10, 8 * R8, 8 * R9}
};

int trace_personality(pid_t pid)
{
    long cs;

    /* Check CS register value,
     * On x86-64 linux this is:
     *  0x33    for long mode (64 bit)
     *  0x23    for compatibility mode (32 bit)
     */
    if (0 > upeek(pid, 8 * CS, &cs))
        return -1;

    switch (cs) {
        case 0x33:
            return 1;
        case 0x23:
            return 0;
        default:
            g_assert_not_reached();
            break;
    }
}

int trace_get_syscall(pid_t pid, long *scno)
{
    int save_errno;

    if (G_UNLIKELY(0 > upeek(pid, ORIG_ACCUM, scno))) {
        save_errno = errno;
        g_info("failed to get syscall number for child %i: %s", pid, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

int trace_set_syscall(pid_t pid, long scno)
{
    int save_errno;

    if (G_UNLIKELY(0 > ptrace(PTRACE_POKEUSER, pid, ORIG_ACCUM, scno))) {
        save_errno = errno;
        g_info("failed to set syscall number to %ld for child %i: %s", scno, pid, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

int trace_get_return(pid_t pid, long *res)
{
    int save_errno;

    if (G_UNLIKELY(0 > upeek(pid, ACCUM, res))) {
        save_errno = errno;
        g_info("failed to get return value for child %i: %s", pid, g_strerror (errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

int trace_set_return(pid_t pid, long val)
{
    int save_errno;

    if (G_UNLIKELY(0 != ptrace(PTRACE_POKEUSER, pid, ACCUM, val))) {
        save_errno = errno;
        g_info("ptrace(PTRACE_POKEUSER,%i,ACCUM,%ld) failed: %s", pid, val, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

int trace_get_arg(pid_t pid, int personality, int arg, long *res)
{
    int save_errno;

    g_assert(arg >= 0 && arg < MAX_ARGS);

    if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][arg], res))) {
        save_errno = errno;
        g_info("failed to get argument %d for child %i: %s", arg, pid, strerror(errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

char *trace_get_path(pid_t pid, int personality, int arg)
{
    int save_errno;
    long addr = 0;

    g_assert(arg >= 0 && arg < MAX_ARGS);

    if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][arg], &addr))) {
        save_errno = errno;
        g_info("failed to get address of argument %d: %s", arg, g_strerror(errno));
        errno = save_errno;
        return NULL;
    }

    char *buf = NULL;
    long len = PATH_MAX;
    for (;;) {
        buf = g_realloc(buf, len * sizeof(char));
        memset(buf, 0, len * sizeof(char));
        if (G_UNLIKELY(0 > umovestr(pid, addr, buf, len))) {
            g_free(buf);
            return NULL;
        }
        else if ('\0' == buf[len - 1])
            break;
        else
            len *= 2;
    }
    return buf;
}

char *trace_get_argv_as_string(pid_t pid, int personality, int arg)
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

    if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][arg], &addr))) {
        save_errno = errno;
        g_info("failed to get address of argument %d: %s", arg, g_strerror(errno));
        errno = save_errno;
        return NULL;
    }

    res = g_string_new("");
    cp.p64 = 1;
    for (sep = "";;sep = ", ") {
        if (umoven(pid, addr, cp.data, (personality == 0) ? 4 : 8) < 0) {
            g_string_append_printf(res, "%#lx", addr);
            return g_string_free(res, FALSE);
        }
        if (personality == 0) /* 32 bit */
            cp.p64 = cp.p32;
        if (cp.p64 == 0)
            break;

        g_string_append(res, sep);

        s = (char *)g_malloc(sizeof(char) * 256);
        s[255] = '\0';
        if (umovestr(pid, cp.p64, s, 256) < 0) {
            g_string_append_printf(res, "%#lx", cp.p64);
            g_free(s);
            break;
        }
        g_string_append_c(res, '"');
        g_string_append(res, s);
        g_string_append_c(res, '"');
        g_free(s);

        addr += (personality == 0) ? 4 : 8;
    }
    if (cp.p64)
        g_string_append_printf(res, "%s...", sep);
    return g_string_free(res, FALSE);
}

int trace_fake_stat(pid_t pid, int personality)
{
    int n, m, save_errno;
    long addr = 0;
    union {
        long val;
        char x[sizeof(long)];
    } u;
    struct stat fakebuf;

    if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][1], &addr))) {
        save_errno = errno;
        g_info("failed to get address of argument %d: %s", 1, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    memset(&fakebuf, 0, sizeof(struct stat));
    fakebuf.st_mode = S_IFCHR | (S_IRUSR | S_IWUSR) | (S_IRGRP | S_IWGRP) | (S_IROTH | S_IWOTH);
    fakebuf.st_rdev = 259; // /dev/null
    fakebuf.st_mtime = -842745600; // ;)

    long *fakeptr = (long *) &fakebuf;
    n = 0;
    m = sizeof(struct stat) / sizeof(long);
    while (n < m) {
        memcpy(u.x, fakeptr, sizeof(long));
        if (0 > ptrace(PTRACE_POKEDATA, pid, addr + n * ADDR_MUL, u.val)) {
            save_errno = errno;
            g_info("failed to set argument 1 to %p for child %i: %s", (void *) fakeptr, pid, g_strerror(errno));
            errno = save_errno;
            return -1;
        }
        ++n;
        ++fakeptr;
    }

    m = sizeof(struct stat) % sizeof(long);
    if (0 != m) {
        memcpy(u.x, fakeptr, m);
        if (G_UNLIKELY(0 > ptrace(PTRACE_POKEDATA, pid, addr + n * ADDR_MUL, u.val))) {
            save_errno = errno;
            g_info("failed to set argument 1 to %p for child %i: %s", (void *) fakeptr, pid, g_strerror(errno));
            errno = save_errno;
            return -1;
        }
    }
    return 0;
}

int trace_decode_socketcall(pid_t pid, int personality)
{
    int save_errno;
    long addr;

     if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][0], &addr))) {
        save_errno = errno;
        g_info("failed to get address of argument 0: %s", g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    return addr;
}

bool trace_get_fd(pid_t pid, int personality, bool decode, long *fd)
{
    int save_errno;
    long args;

    g_assert(fd != NULL);

    if (decode) {
        if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][1], &args))) {
            save_errno = errno;
            g_info("failed to get address of argument 1: %s", g_strerror(errno));
            errno = save_errno;
            return NULL;
        }
        if (umove(pid, args, fd) < 0) {
            save_errno = errno;
            g_info("failed to decode argument 0: %s", g_strerror(errno));
            errno = save_errno;
            return false;
        }
        return true;
    }

    if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][0], fd))) {
        save_errno = errno;
        g_info("failed to get address of argument 0: %s", g_strerror(errno));
        errno = save_errno;
        return false;
    }
    return true;
}

struct sydbox_addr *trace_get_addr(pid_t pid, int personality, int narg,
        bool decode, long *fd)
{
    int save_errno;
    long addr, addrlen, args;
    union {
        char pad[128];
        struct sockaddr sa;
        struct sockaddr_un sa_un;
        struct sockaddr_in sa_in;
#if HAVE_IPV6
        struct sockaddr_in6 sa6;
#endif /* HAVE_IPV6 */
    } addrbuf;
    struct sydbox_addr *saddr;

    if (decode) {
        unsigned int iaddr, iaddrlen;

        if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][1], &args))) {
            save_errno = errno;
            g_info("failed to get address of argument 1: %s", g_strerror(errno));
            errno = save_errno;
            return NULL;
        }
        if (fd != NULL) {
            if (umove(pid, args, fd) < 0) {
                save_errno = errno;
                g_info("failed to decode argument 0: %s", g_strerror(errno));
                errno = save_errno;
                return NULL;
            }
        }
        args += narg * sizeof(unsigned int);
        if (umove(pid, args, &iaddr) < 0) {
            save_errno = errno;
            g_info("failed to decode argument %d: %s", narg, g_strerror(errno));
            errno = save_errno;
            return NULL;
        }
        args += sizeof(unsigned int);
        if (umove(pid, args, &iaddrlen) < 0) {
            save_errno = errno;
            g_info("failed to decode argument %d: %s", narg + 1, g_strerror(errno));
            errno = save_errno;
            return NULL;
        }
        addr = iaddr;
        addrlen = iaddrlen;
    }
    else {
        if (fd != NULL) {
            if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][0], fd))) {
                save_errno = errno;
                g_info("failed to get address of argument 0: %s", g_strerror(errno));
                errno = save_errno;
                return NULL;
            }
        }
        if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][narg], &addr))) {
            save_errno = errno;
            g_info("failed to get address of argument %d: %s", narg, g_strerror(errno));
            errno = save_errno;
            return NULL;
        }
        if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][narg + 1], &addrlen))) {
            save_errno = errno;
            g_info("failed to get address of argument %d: %s", narg + 1, g_strerror(errno));
            errno = save_errno;
            return NULL;
        }
    }

    saddr = g_new0(struct sydbox_addr, 1);
    if (addr == 0) {
        saddr->family = -1;
        return saddr;
    }
    if (addrlen < 2 || (unsigned long)addrlen > sizeof(addrbuf))
        addrlen = sizeof(addrbuf);

    memset(&addrbuf, 0, sizeof(addrbuf));
    if (umoven(pid, addr, addrbuf.pad, addrlen) < 0) {
        save_errno = errno;
        g_info("failed to get socket address: %s", g_strerror(errno));
        errno = save_errno;
        return NULL;
    }
    addrbuf.pad[sizeof(addrbuf.pad) - 1] = '\0';

    saddr->family = addrbuf.sa.sa_family;

    switch (addrbuf.sa.sa_family) {
        case AF_UNIX:
            saddr->u.saun.exact = true;
            if (addrlen == 2)
                saddr->u.saun.sun_path[0] = '\0';
            else if (addrbuf.sa_un.sun_path[0] != '\0')
                strncpy(saddr->u.saun.sun_path, addrbuf.sa_un.sun_path, strlen(addrbuf.sa_un.sun_path) + 1);
            else {
                saddr->u.saun.abstract = true;
                strncpy(saddr->u.saun.sun_path, addrbuf.sa_un.sun_path + 1, strlen(addrbuf.sa_un.sun_path + 1) + 1);
            }
            break;
        case AF_INET:
            saddr->u.sa.port[0] = ntohs(addrbuf.sa_in.sin_port);
            saddr->u.sa.port[1] = saddr->u.sa.port[0];
            saddr->u.sa.netmask = 32;
            memcpy(&saddr->u.sa.sin_addr, &addrbuf.sa_in.sin_addr, sizeof(struct in_addr));
            break;
#if HAVE_IPV6
        case AF_INET6:
            saddr->u.sa6.port[0] = ntohs(addrbuf.sa6.sin6_port);
            saddr->u.sa6.port[1] = saddr->u.sa6.port[0];
            saddr->u.sa6.netmask = 64;
            memcpy(&saddr->u.sa6.sin6_addr, &addrbuf.sa6.sin6_addr, sizeof(struct in6_addr));
            break;
#endif /* HAVE_IPV6 */
        default:
            break;
    }
    return saddr;
}

