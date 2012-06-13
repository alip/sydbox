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

#ifndef _ATFILE_SOURCE
#define _ATFILE_SOURCE
#endif // !_ATFILE_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <glib.h>
#include <pinktrace/pink.h>

#include "syd-config.h"
#include "syd-flags.h"
#include "syd-dispatch.h"
#include "syd-log.h"
#include "syd-net.h"
#include "syd-path.h"
#include "syd-pink.h"
#include "syd-proc.h"
#include "syd-syscall.h"
#include "syd-utils.h"
#include "syd-wrappers.h"

#if SYDBOX_HAVE_IPV6
#define IS_SUPPORTED_FAMILY(f)      ((f) == AF_UNIX || (f) == AF_INET || (f) == AF_INET6)
#else
#define IS_SUPPORTED_FAMILY(f)      ((f) == AF_UNIX || (f) == AF_INET)
#endif /* SYDBOX_HAVE_IPV6 */
#define IS_NET_CALL(fl)             ((fl) & (BIND_CALL | CONNECT_CALL | SENDTO_CALL | DECODE_SOCKETCALL))

#define MODE_STRING(fl) ((fl) & (OPEN_MODE | OPEN_MODE_AT) ? "O_WRONLY/O_RDWR" : "...")

enum {
    RS_ALLOW,
    RS_NOWRITE,
    RS_MAGIC,
    RS_DENY,
    RS_ERROR = EX_SOFTWARE
};

struct checkdata {
    gint result;                // Check result
    gint save_errno;            // errno when the result is RS_ERROR

    bool resolve;               // true if the system call resolves paths
    glong open_flags;           // flags argument of open()/openat()
    glong access_flags;         // flags argument of access()/faccessat()
    gchar *sargv;               // argv[] list of execve() call stringified
    gchar *dirfdlist[2];        // dirfd arguments (resolved)
    gchar *pathlist[4];         // Path arguments
    gchar *rpathlist[4];        // Path arguments (canonicalized)

    long subcall;               // Socketcall() subcall
    struct sydbox_addr *addr;   // Destination address of socket call
};

static long sno;
static int sflags;
static const char *sname;

/* Receive the path argument at position narg of child with given pid and
 * update data.
 * Returns FALSE and sets data->result to RS_ERROR and data->save_errno to
 * errno on failure.
 * Returns TRUE and updates data->pathlist[narg] on success.
 */
static bool syscall_get_path(pid_t pid, pink_bitness_t bitness, int narg, struct checkdata *data)
{
    errno = 0;
    data->pathlist[narg] = pink_decode_string_persistent(pid, bitness, narg);
    if (G_UNLIKELY(NULL == data->pathlist[narg])) {
        data->result = RS_ERROR;
        if (errno) {
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to grab string from argument %d: %s", narg, g_strerror(errno));
            else
                g_warning("failed to grab string from argument %d: %s", narg, g_strerror(errno));
        }
        else {
            data->save_errno = EFAULT;
            g_debug("path argument %d is NULL", narg);
        }
        return false;
    }
    g_debug("path argument %d is `%s'", narg, data->pathlist[narg]);
    return true;
}

/* Receive dirfd argument at position narg of the given child and update data.
 * Returns FALSE and sets data->result to RS_ERROR and data->save_errno to
 * errno on failure.
 * If dirfd is AT_FDCWD it copies child->cwd to data->dirfdlist[narg].
 * Otherwise tries to determine the directory using proc_getdir().
 * If proc_getdir() fails it sets data->result to RS_DENY and child->retval to
 * -errno and returns FALSE.
 * On success TRUE is returned and data->dirfdlist[narg] contains the directory
 * information about dirfd. This string should be freed after use.
 */
static bool syscall_get_dirfd(struct tchild *child, int narg, struct checkdata *data)
{
    long dfd;

    if (G_UNLIKELY(!pink_util_get_arg(child->pid, child->bitness, narg, &dfd))) {
        data->result = RS_ERROR;
        data->save_errno = errno;
        if (ESRCH == errno)
            g_debug("failed to get dirfd from argument %d: %s", narg, g_strerror(errno));
        else
            g_warning("failed to get dirfd from argument %d: %s", narg, g_strerror(errno));
        return false;
    }

    if (AT_FDCWD != dfd) {
        data->dirfdlist[narg] = proc_getdir(child->pid, dfd);
        if (NULL == data->dirfdlist[narg]) {
            data->result = RS_DENY;
            child->retval = -errno;
            g_debug("proc_getdir() failed: %s", g_strerror(errno));
            g_debug("denying access to system call %lu(%s)", sno, sname);
            return false;
        }
    }
    else
        data->dirfdlist[narg] = g_strdup(child->cwd);
    return true;
}

static bool syscall_decode_net(struct tchild *child, struct checkdata *data)
{
    if (!pink_decode_socket_call(child->pid, child->bitness, &data->subcall)) {
        data->result = RS_ERROR;
        data->save_errno = errno;
        return false;
    }

    sname = pink_name_socket_subcall(data->subcall);
    g_debug("Decoded socket subcall is %ld(%s)", data->subcall, sname);
    if (data->subcall == PINK_SOCKET_SUBCALL_BIND || data->subcall == PINK_SOCKET_SUBCALL_CONNECT) {
        data->addr = pinkw_get_socket_addr(child->pid, child->bitness, 1, NULL);
        if (data->addr == NULL) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            return false;
        }
    }
    else if (data->subcall == PINK_SOCKET_SUBCALL_SENDTO) {
        data->addr = pinkw_get_socket_addr(child->pid, child->bitness, 4, NULL);
        if (data->addr == NULL) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            return false;
        }
    }
    return true;
}

static bool syscall_getaddr_net(struct tchild *child, struct checkdata *data)
{
    if (sflags & DECODE_SOCKETCALL)
        return syscall_decode_net(child, data);
    else if (sflags & (BIND_CALL | CONNECT_CALL))
        data->addr = pinkw_get_socket_addr(child->pid, child->bitness, 1, NULL);
    else if (sflags & SENDTO_CALL)
        data->addr = pinkw_get_socket_addr(child->pid, child->bitness, 4, NULL);
    else
        return true;

    if (data->addr == NULL) {
        data->result = RS_ERROR;
        data->save_errno = errno;
        return false;
    }
    return true;
}

/* Initial callback for system call handler.
 * Updates struct checkdata with path and dirfd information.
 */
static void syscall_check_start(G_GNUC_UNUSED context_t *ctx, struct tchild *child, struct checkdata *data)
{
    g_debug("starting check for system call %lu(%s), child %i", sno, sname, child->pid);

    if (sflags & (CHECK_PATH | MAGIC_STAT)) {
        if (!syscall_get_path(child->pid, child->bitness, 0, data))
            return;
    }
    if (sflags & CHECK_PATH2) {
        if (!syscall_get_path(child->pid, child->bitness, 1, data))
            return;
    }
    if (sflags & CHECK_PATH_AT) {
        if (!syscall_get_path(child->pid, child->bitness, 1, data))
            return;
        if (!g_path_is_absolute(data->pathlist[1]) && !syscall_get_dirfd(child, 0, data))
            return;
    }
    if (sflags & CHECK_PATH_AT1) {
        if (!syscall_get_path(child->pid, child->bitness, 2, data))
            return;
        if (!g_path_is_absolute(data->pathlist[2]) && !syscall_get_dirfd(child, 1, data))
            return;
    }
    if (sflags & CHECK_PATH_AT2) {
        if (!syscall_get_path(child->pid, child->bitness, 3, data))
            return;
        if (!g_path_is_absolute(data->pathlist[3]) && !syscall_get_dirfd(child, 2, data))
            return;
    }
#if 0
    if (child->sandbox->exec && sflags & EXEC_CALL) {
#endif
    if (sflags & EXEC_CALL) {
        if (!syscall_get_path(child->pid, child->bitness, 0, data))
            return;
        if ((data->sargv = pinkw_stringify_argv(child->pid, child->bitness, 1)) == NULL)
            return;
        g_string_printf(child->lastexec, "execve(\"%s\", [%s])", data->pathlist[0], data->sargv);
    }
    if (child->sandbox->network) {
        if (!syscall_getaddr_net(child, data))
            return;
    }
}

/* Second callback for system call handler
 * Checks the flag arguments of system calls.
 * If data->result isn't RS_ALLOW, which means an error has occured in a
 * previous callback or a decision has been made, it does nothing and simply
 * returns.
 * If the system call isn't one of open, openat, access or accessat, it does
 * nothing and simply returns.
 * If an error occurs during flag checking it sets data->result to RS_ERROR,
 * data->save_errno to errno and returns.
 * If the flag doesn't have O_CREAT, O_WRONLY or O_RDWR set for system call
 * open or openat it sets data->result to RS_NOWRITE and returns.
 * If the flag doesn't have W_OK set for system call access or accessat it
 * sets data->result to RS_NOWRITE and returns.
 */
static void syscall_check_flags(struct tchild *child, struct checkdata *data)
{
    if (G_UNLIKELY(RS_ALLOW != data->result))
        return;
    else if (!(sflags & (OPEN_MODE | OPEN_MODE_AT | ACCESS_MODE | ACCESS_MODE_AT)))
        return;

    if (sflags & (OPEN_MODE | OPEN_MODE_AT)) {
        int arg = sflags & OPEN_MODE ? 1 : 2;
        if (G_UNLIKELY(!pink_util_get_arg(child->pid, child->bitness, arg, &data->open_flags))) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to get argument %d: %s", arg, g_strerror(errno));
            else
                g_warning("failed to get argument %d: %s", arg, g_strerror(errno));
            return;
        }
        if (!(data->open_flags & (O_CREAT | O_WRONLY | O_RDWR)))
            data->result = RS_NOWRITE;
    }
    else {
        int arg = sflags & ACCESS_MODE ? 1 : 2;
        if (G_UNLIKELY(!pink_util_get_arg(child->pid, child->bitness, arg, &data->access_flags))) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to get argument %d: %s", arg, g_strerror(errno));
            else
                g_warning("failed to get argument %d: %s", arg, g_strerror(errno));
            return;
        }
        if (!(data->access_flags & W_OK))
            data->result = RS_NOWRITE;
    }
}

/* Checks for magic stat() calls.
 * If the stat() call is magic, this function calls trace_fake_stat() to fake
 * the stat buffer and sets data->result to RS_DENY and child->retval to 0.
 * If trace_fake_stat() fails it sets data->result to RS_ERROR and
 * data->save_errno to errno.
 * If the stat() call isn't magic, this function does nothing.
 */
static void syscall_magic_stat(struct tchild *child, struct checkdata *data)
{
    char *path = data->pathlist[0];
    const char *rpath;
    char *rpath_sanitized;
    GSList *walk, *whitelist;
    char **expaddr;
    struct sydbox_addr *addr;

    g_debug("checking if stat(\"%s\") is magic", path);
    if (G_LIKELY(!path_magic_dir(path))) {
        g_debug("stat(\"%s\") not magic", path);
        return;
    }

    if (path_magic_on(path)) {
        data->result = RS_MAGIC;
        child->sandbox->path = true;
        g_info("path sandboxing is now enabled for child %i", child->pid);
    }
    else if (path_magic_off(path)) {
        data->result = RS_MAGIC;
        child->sandbox->path = false;
        g_info("path sandboxing is now disabled for child %i", child->pid);
    }
    else if (path_magic_toggle(path)) {
        data->result = RS_MAGIC;
        child->sandbox->path = !(child->sandbox->path);
        g_info("path sandboxing is now %sabled for child %i", child->sandbox->path ? "en" : "dis", child->pid);
    }
    else if (path_magic_lock(path)) {
        data->result = RS_MAGIC;
        child->sandbox->lock = LOCK_SET;
        g_info("access to magic commands is now denied for child %i", child->pid);
    }
    else if (path_magic_exec_lock(path)) {
        data->result = RS_MAGIC;
        child->sandbox->lock = LOCK_PENDING;
        g_info("access to magic commands will be denied on execve() for child %i", child->pid);
    }
    else if (path_magic_wait_all(path)) {
        data->result = RS_MAGIC;
        sydbox_config_set_wait_all(true);
        g_info("tracing will be finished when all children exit");
    }
    else if (path_magic_wait_eldest(path)) {
        data->result = RS_MAGIC;
        sydbox_config_set_wait_all(false);
        g_info("tracing will be finished when the eldest child exits");
    }
    else if (path_magic_wrap_lstat(path)) {
        data->result = RS_MAGIC;
        sydbox_config_set_wrap_lstat(true);
        g_info("lstat() calls will now be wrapped");
    }
    else if (path_magic_nowrap_lstat(path)) {
        data->result = RS_MAGIC;
        sydbox_config_set_wrap_lstat(false);
        g_info("lstat() calls will now not be wrapped");
    }
    else if (path_magic_write(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_WRITE) - 1;
        pathnode_new(&(child->sandbox->write_prefixes), rpath, true);
        g_info("approved addwrite(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_rmwrite(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_RMWRITE) - 1;
        rpath_sanitized = sydbox_compress_path(rpath);
        if (NULL != child->sandbox->write_prefixes)
            pathnode_delete(&(child->sandbox->write_prefixes), rpath_sanitized);
        g_info("approved rmwrite(\"%s\") for child %i", rpath_sanitized, child->pid);
        g_free(rpath_sanitized);
    }
    else if (path_magic_sandbox_exec(path)) {
        data->result = RS_MAGIC;
        child->sandbox->exec = true;
        g_info("execve(2) sandboxing is now enabled for child %i", child->pid);
    }
    else if (path_magic_sandunbox_exec(path)) {
        data->result = RS_MAGIC;
        child->sandbox->exec = false;
        g_info("execve(2) sandboxing is now disabled for child %i", child->pid);
    }
    else if (path_magic_addexec(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_ADDEXEC) - 1;
        pathnode_new(&(child->sandbox->exec_prefixes), rpath, true);
        g_info("approved addexec(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_rmexec(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_RMEXEC) - 1;
        rpath_sanitized = sydbox_compress_path(rpath);
        if (NULL != child->sandbox->exec_prefixes)
            pathnode_delete(&(child->sandbox->exec_prefixes), rpath_sanitized);
        g_info("approved rmexec(\"%s\") for child %i", rpath_sanitized, child->pid);
        g_free(rpath_sanitized);
    }
    else if (path_magic_sandbox_net(path)) {
        data->result = RS_MAGIC;
        child->sandbox->network = true;
        g_info("network sandboxing is now enabled for child %i", child->pid);
    }
    else if (path_magic_sandunbox_net(path)) {
        data->result = RS_MAGIC;
        child->sandbox->network = false;
        g_info("network sandboxing is now disabled for child %i", child->pid);
    }
    else if (path_magic_addfilter(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_ADDFILTER) - 1;
        sydbox_config_addfilter(rpath);
        g_info("approved addfilter(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_rmfilter(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_RMFILTER) - 1;
        sydbox_config_rmfilter(rpath);
        g_info("approved rmfilter(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_addfilter_exec(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_ADDFILTER_EXEC) - 1;
        sydbox_config_addfilter_exec(rpath);
        g_info("approved addfilter_exec(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_rmfilter_exec(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_RMFILTER_EXEC) - 1;
        sydbox_config_rmfilter_exec(rpath);
        g_info("approved rmfilter_exec(\"%s\") for child %i", rpath, child->pid);
    }
    else if (path_magic_addfilter_net(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_ADDFILTER_NET) - 1;
        expaddr = address_alias_expand(rpath, true);
        for (unsigned i = 0; expaddr[i]; i++) {
            if ((addr = address_from_string(expaddr[i], true)) == NULL)
                g_warning("malformed filter address `%s'", expaddr[i]);
            else {
                sydbox_config_addfilter_net(addr);
                g_free(addr);
                g_info("approved addfilter_net(\"%s\") for child %i", expaddr[i], child->pid);
            }
        }
        g_strfreev(expaddr);
    }
    else if (path_magic_rmfilter_net(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_RMFILTER_NET) - 1;
        expaddr = address_alias_expand(rpath, true);
        for (unsigned i = 0; expaddr[i]; i++) {
            if ((addr = address_from_string(expaddr[i], true)) == NULL)
                g_warning("malformed filter address `%s'", expaddr[i]);
            else {
                sydbox_config_rmfilter_net(addr);
                g_free(addr);
                g_info("approved rmfilter_net(\"%s\") for child %i", expaddr[i], child->pid);
            }
        }
        g_strfreev(expaddr);
    }
    else if (path_magic_net_whitelist_bind(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_NET_WHITELIST_BIND) - 1;
        expaddr = address_alias_expand(rpath, true);
        for (unsigned i = 0; expaddr[i]; i++) {
            if ((addr = address_from_string(expaddr[i], true)) == NULL)
                g_warning("malformed whitelist address `%s'", expaddr[i]);
            else {
                whitelist = sydbox_config_get_network_whitelist_bind();
                whitelist = g_slist_prepend(whitelist, addr);
                sydbox_config_set_network_whitelist_bind(whitelist);
            }
        }
        g_strfreev(expaddr);
    }
    else if (path_magic_net_unwhitelist_bind(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_NET_UNWHITELIST_BIND) - 1;
        expaddr = address_alias_expand(rpath, true);
        for (unsigned i = 0; expaddr[i]; i++) {
            if ((addr = address_from_string(expaddr[i], false)) == NULL)
                g_warning("malformed whitelist address `%s'", expaddr[i]);
            else {
                whitelist = sydbox_config_get_network_whitelist_bind();
                for (walk = whitelist; walk != NULL; walk = g_slist_next(walk)) {
                    if (address_cmp(walk->data, addr)) {
                        whitelist = g_slist_remove_link(whitelist, walk);
                        sydbox_config_set_network_whitelist_bind(whitelist);
                        g_free(walk->data);
                        g_slist_free(walk);
                        g_info("approved unwhitelist/bind(\"%s\") for child %i", expaddr[i], child->pid);
                        break;
                    }
                }
            }
        }
        g_strfreev(expaddr);
    }
    else if (path_magic_net_whitelist_connect(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_NET_WHITELIST_CONNECT) - 1;
        expaddr = address_alias_expand(rpath, true);
        for (unsigned i = 0; expaddr[i]; i++) {
            if ((addr = address_from_string(expaddr[i], true)) == NULL)
                g_warning("malformed whitelist address `%s'", expaddr[i]);
            else {
                whitelist = sydbox_config_get_network_whitelist_connect();
                whitelist = g_slist_prepend(whitelist, addr);
                sydbox_config_set_network_whitelist_connect(whitelist);
            }
        }
        g_strfreev(expaddr);
    }
    else if (path_magic_net_unwhitelist_connect(path)) {
        data->result = RS_MAGIC;
        rpath = path + sizeof(CMD_NET_UNWHITELIST_CONNECT) - 1;
        expaddr = address_alias_expand(rpath, true);
        for (unsigned i = 0; expaddr[i]; i++) {
            if ((addr = address_from_string(expaddr[i], false)) == NULL)
                g_warning("malformed whitelist address `%s'", rpath);
            else {
                whitelist = sydbox_config_get_network_whitelist_connect();
                for (walk = whitelist; walk != NULL; walk = g_slist_next(walk)) {
                    if (address_cmp(walk->data, addr)) {
                        whitelist = g_slist_remove_link(whitelist, walk);
                        sydbox_config_set_network_whitelist_connect(whitelist);
                        g_free(walk->data);
                        g_slist_free(walk);
                        g_info("approved unwhitelist/connect(\"%s\") for child %i", expaddr[i], child->pid);
                    }
                }
            }
        }
        g_strfreev(expaddr);
    }
    else if (child->sandbox->path || !path_magic_enabled(path))
        data->result = RS_MAGIC;

    if (data->result == RS_MAGIC) {
        g_debug("stat(\"%s\") is magic, encoding stat buffer", path);
        if (G_UNLIKELY(!pinkw_encode_stat(child->pid, child->bitness))) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to encode stat buffer: %s", g_strerror(errno));
            else
                g_warning("failed to encode stat buffer: %s", g_strerror(errno));
        }
        else {
            data->result = RS_DENY;
            child->retval = 0;
        }
    }
    else
        g_debug("stat(\"%s\") is not magic", path);
}

/* Third callback for system call handler.
 * Checks for magic calls.
 * If data->result isn't RS_ALLOW, which means an error has occured in a
 * previous callback or a decision has been made, it does nothing and simply
 * returns.
 * If child->sandbox->lock is set to LOCK_SET which means magic calls are
 * locked, it does nothing and simply returns.
 * If the system call isn't stat(), it does nothing and simply returns.
 * Otherwise it calls systemcall_magic_stat()
 */
static void syscall_check_magic(struct tchild *child, struct checkdata *data)
{

    if (G_UNLIKELY(RS_ALLOW != data->result))
        return;
    else if (LOCK_SET == child->sandbox->lock) {
        g_debug("Lock is set for child %i, skipping magic checks", child->pid);
        return;
    }
    else if (!(sflags & MAGIC_STAT))
        return;

    syscall_magic_stat(child, data);
}

/* Fourth callback for systemcall handler.
 * Checks whether symlinks should be resolved for the given system call
 * If data->result isn't RS_ALLOW, which means an error has occured in a
 * previous callback or a decision has been made, it does nothing and simply
 * returns.
 * If child->sandbox->path is false it does nothing and simply returns.
 * If everything was successful this function sets data->resolve to a boolean
 * which gives information about whether the symlinks should be resolved.
 * On failure this function sets data->result to RS_ERROR and data->save_errno
 * to errno.
 */
static void syscall_check_resolve(struct tchild *child, struct checkdata *data)
{
    if (G_UNLIKELY(RS_ALLOW != data->result))
        return;
    else if (child->sandbox->exec && sflags & EXEC_CALL) {
        data->resolve = true;
        return;
    }
    else if (child->sandbox->network && IS_NET_CALL(sflags)) {
        data->resolve = true;
        return;
    }
    else if (!child->sandbox->path)
        return;

    g_debug("deciding whether we should resolve symlinks for system call %lu(%s), child %i",
            sno, sname, child->pid);
    if (sflags & DONT_RESOLV)
        data->resolve = false;
    else if (sflags & IF_AT_SYMLINK_FOLLOW4) {
        long symflags;
        if (G_UNLIKELY(!pink_util_get_arg(child->pid, child->bitness, 4, &symflags))) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to get argument 4: %s", g_strerror(errno));
            else
                g_warning("failed to get argument 4: %s", g_strerror(errno));
            return;
        }
        data->resolve = symflags & AT_SYMLINK_FOLLOW ? true : false;
    }
    else if (sflags & IF_AT_SYMLINK_NOFOLLOW3 || sflags & IF_AT_SYMLINK_NOFOLLOW4) {
        long symflags;
        int arg = sflags & IF_AT_SYMLINK_NOFOLLOW3 ? 3 : 4;
        if (G_UNLIKELY(!pink_util_get_arg(child->pid, child->bitness, arg, &symflags))) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to get argument %d: %s", arg, g_strerror(errno));
            else
                g_warning("failed to get argument %d: %s", arg, g_strerror(errno));
            return;
        }
        data->resolve = symflags & AT_SYMLINK_NOFOLLOW ? false : true;
    }
    else if (sflags & IF_AT_REMOVEDIR2) {
        long rmflags;
        if (G_UNLIKELY(!pink_util_get_arg(child->pid, child->bitness, 2, &rmflags))) {
            data->result = RS_ERROR;
            data->save_errno = errno;
            if (ESRCH == errno)
                g_debug("failed to get argument 2: %s", g_strerror(errno));
            else
                g_warning("failed to get argument 2: %s", g_strerror(errno));
            return;
        }
        data->resolve = rmflags & AT_REMOVEDIR ? true : false;
    }
    else
        data->resolve = true;
    g_debug("decided %sto resolve symlinks for system call %lu(%s), child %i",
            data->resolve ? "" : "not ", sno, sname, child->pid);
}

/* Resolves path for system calls
 * This function calls canonicalize_filename_mode() after sanitizing path
 * On success it returns resolved path.
 * On failure it sets data->result to RS_DENY and child->retval to -errno.
 */
static gchar *syscall_resolvepath(struct tchild *child, struct checkdata *data, int narg, bool isat)
{
    bool maycreat;
    int mode;
    char *path, *path_sanitized, *resolved_path;

    if (data->open_flags & O_CREAT)
        maycreat = true;
    else if (0 == narg && sflags & (CAN_CREAT | MUST_CREAT))
        maycreat = true;
    else if (1 == narg && sflags & (CAN_CREAT2 | MUST_CREAT2))
        maycreat = true;
    else if (1 == narg && isat && sflags & (CAN_CREAT_AT | MUST_CREAT_AT))
        maycreat = true;
    else if (2 == narg && isat && sflags & MUST_CREAT_AT1)
        maycreat = true;
    else if (3 == narg && sflags & (CAN_CREAT_AT2 | MUST_CREAT_AT2))
        maycreat = true;
    else if (-1 == narg) /* Non-abstract UNIX socket */
        maycreat = true;
    else
        maycreat = false;
    mode = maycreat ? CAN_ALL_BUT_LAST : CAN_EXISTING;

    if (-1 == narg) {
        g_assert(data->addr != NULL);
        g_assert(data->addr->family == AF_UNIX);
        g_assert(!data->addr->u.saun.abstract);
        path = data->addr->u.saun.sun_path;
    }
    else
        path = data->pathlist[narg];

    if (!g_path_is_absolute(path)) {
        char *absdir, *abspath;
        if (isat && NULL != data->dirfdlist[narg - 1]) {
            absdir = data->dirfdlist[narg - 1];
            g_debug("adding dirfd `%s' to `%s' to make it an absolute path", absdir, path);
        }
        else {
            absdir = child->cwd;
            g_debug("adding current working directory `%s' to `%s' to make it an absolute path", absdir, path);
        }

        abspath = g_build_path(G_DIR_SEPARATOR_S, absdir, path, NULL);
        path_sanitized = sydbox_compress_path(abspath);
        g_free(abspath);
    }
    else
        path_sanitized = sydbox_compress_path(path);

#ifdef HAVE_PROC_SELF
    /* Special case for /proc/self.
     * This symbolic link resolves to /proc/PID, if we let
     * canonicalize_filename_mode() resolve this, we'll get a different result.
     */
    if (0 == strncmp(path_sanitized, "/proc/self", 10)) {
        g_debug("substituting /proc/self with /proc/%i", child->pid);
        char *tmp = g_malloc(strlen(path_sanitized) + 32);
        snprintf(tmp, strlen(path_sanitized) + 32, "/proc/%i/%s", child->pid, path_sanitized + 10);
        g_free(path_sanitized);
        path_sanitized = sydbox_compress_path(tmp);
        g_free(tmp);
    }
#endif

    g_debug("mode is %s resolve is %s", maycreat ? "CAN_ALL_BUT_LAST" : "CAN_EXISTING",
                                        data->resolve ? "TRUE" : "FALSE");
    resolved_path = canonicalize_filename_mode(path_sanitized, mode, data->resolve);
    if (NULL == resolved_path) {
        data->result = RS_DENY;
        child->retval = -errno;
        g_debug("canonicalize_filename_mode() failed for `%s': %s", path, g_strerror(errno));
    }
    g_free(path_sanitized);
    return resolved_path;
}

/* Fifth callback for system call handler.
 * Canonicalizes path arguments.
 * If data->result isn't RS_ALLOW, which means an error has occured in a
 * previous callback or a decision has been made, it does nothing and simply
 * returns.
 * If child->sandbox->path is false it does nothing and simply returns.
 */
static void syscall_check_canonicalize(G_GNUC_UNUSED context_t *ctx, struct tchild *child,
        struct checkdata *data)
{
    if (G_UNLIKELY(RS_ALLOW != data->result))
        return;

    if (child->sandbox->exec && sflags & EXEC_CALL) {
        g_debug("canonicalizing `%s' for system call %lu(%s), child %i", data->pathlist[0],
                sno, sname, child->pid);
        data->rpathlist[0] = syscall_resolvepath(child, data, 0, false);
        if (NULL != data->rpathlist[0])
            g_debug("canonicalized `%s' to `%s'", data->pathlist[0], data->rpathlist[0]);
        return;
    }
    if (child->sandbox->network &&
            IS_NET_CALL(sflags) &&
            data->addr != NULL &&
            data->addr->family == AF_UNIX &&
            !data->addr->u.saun.abstract) {
        g_debug("canonicalizing `%s' for system call %lu(%s), child %i",
                data->addr->u.saun.sun_path, sno, sname, child->pid);
        data->addr->u.saun.rsun_path = syscall_resolvepath(child, data, -1, false);
        if (NULL != data->addr->u.saun.rsun_path)
            g_debug("canonicalized `%s' to `%s'", data->addr->u.saun.sun_path, data->addr->u.saun.rsun_path);
        return;
    }

    if (!child->sandbox->path)
        return;
    if (sflags & CHECK_PATH) {
        g_debug("canonicalizing `%s' for system call %lu(%s), child %i", data->pathlist[0],
                sno, sname, child->pid);
        data->rpathlist[0] = syscall_resolvepath(child, data, 0, false);
        if (NULL == data->rpathlist[0])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[0], data->rpathlist[0]);
    }
    if (sflags & CHECK_PATH2) {
        g_debug("canonicalizing `%s' for system call %lu(%s), child %i", data->pathlist[1],
                sno, sname, child->pid);
        data->rpathlist[1] = syscall_resolvepath(child, data, 1, false);
        if (NULL == data->rpathlist[1])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[1], data->rpathlist[1]);
    }
    if (sflags & CHECK_PATH_AT) {
        g_debug("canonicalizing `%s' for system call %lu(%s), child %i", data->pathlist[1],
                sno, sname, child->pid);
        data->rpathlist[1] = syscall_resolvepath(child, data, 1, true);
        if (NULL == data->rpathlist[1])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[1], data->rpathlist[1]);
    }
    if (sflags & CHECK_PATH_AT1) {
        g_debug("canonicalizing `%s' for system call %lu(%s), child %i", data->pathlist[2],
                sno, sname, child->pid);
        data->rpathlist[2] = syscall_resolvepath(child, data, 2, true);
        if (NULL == data->rpathlist[2])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[2], data->rpathlist[2]);
    }
    if (sflags & CHECK_PATH_AT2) {
        g_debug("canonicalizing `%s' for system call %lu(%s), child %i", data->pathlist[3],
                sno, sname, child->pid);
        data->rpathlist[3] = syscall_resolvepath(child, data, 3, true);
        if (NULL == data->rpathlist[3])
            return;
        else
            g_debug("canonicalized `%s' to `%s'", data->pathlist[3], data->rpathlist[3]);
    }
}

static int syscall_handle_create(struct tchild *child, struct checkdata *data, int narg)
{
    char *path;
    struct stat buf;

    path = data->rpathlist[narg];
    if ((narg == 0 && sflags & MUST_CREAT) ||
            (narg == 1 && sflags & (MUST_CREAT2 | MUST_CREAT_AT)) ||
            (narg == 3 && sflags & MUST_CREAT_AT2)) {
        g_debug("system call %lu(%s) has one of MUST_CREAT* flags set, checking if `%s' exists",
                sno, sname, path);
        if (0 == stat(path, &buf)) {
            /* The system call _has_ to create the path but it exists.
             * Deny the system call and set errno to EEXIST but don't throw
             * an access violation.
             * Useful for cases like mkdir -p a/b/c.
             */
            g_debug("`%s' exists, system call %lu(%s) will fail with EEXIST", path, sno, sname);
            g_debug("denying system call %lu(%s) and failing with EEXIST without violation", sno, sname);
            data->result = RS_DENY;
            child->retval = -EEXIST;
            return 1;
        }
    }
    return 0;
}

static void syscall_handle_path(struct tchild *child, struct checkdata *data, int narg)
{
    char *path = data->rpathlist[narg];

    g_debug("checking `%s' for write access", path);

    if (G_UNLIKELY(!pathlist_check(child->sandbox->write_prefixes, path))) {
        if (syscall_handle_create(child, data, narg))
            return;

        data->result = RS_DENY;
        child->retval = -EPERM;

        /* Don't raise access violations for access(2) system call.
         * Silently deny it instead.
         */
        if (sflags & ACCESS_MODE)
            return;

        switch (narg) {
            case 0:
                sydbox_access_violation_path(child, path, "%s(\"%s\", %s)",
                        sname, path, MODE_STRING(sflags));
                break;
            case 1:
                sydbox_access_violation_path(child, path, "%s(?, \"%s\", %s)",
                        sname, path, MODE_STRING(sflags));
                break;
            case 2:
                sydbox_access_violation_path(child, path, "%s(?, ?, \"%s\", %s)",
                        sname, path, MODE_STRING(sflags));
                break;
            case 3:
                sydbox_access_violation_path(child, path, "%s(?, ?, ?, \"%s\", %s)",
                        sname, path, MODE_STRING(sflags));
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }
}

static void syscall_handle_net(struct tchild *child, struct checkdata *data)
{
    bool isbind, violation;
    char ip[100] = { 0 };
    GSList *whitelist, *walk;
    struct sydbox_addr *addr;

    isbind = ((sflags & BIND_CALL) || ((sflags & DECODE_SOCKETCALL) && data->subcall == PINK_SOCKET_SUBCALL_BIND));
    whitelist = isbind
        ? sydbox_config_get_network_whitelist_bind()
        : sydbox_config_get_network_whitelist_connect();

    violation = true;
    for (walk = whitelist; walk != NULL; walk = g_slist_next(walk)) {
        addr = (struct sydbox_addr *)walk->data;
        if (address_has(addr, data->addr)) {
            /* Check port range for NET_FAMILY. */
            switch (addr->family) {
                case AF_UNIX:
                    violation = false;
                    break;
                case AF_INET:
                    if (data->addr->u.sa.port[0] >= addr->u.sa.port[0] &&
                            data->addr->u.sa.port[1] <= addr->u.sa.port[1])
                        violation = false;
                    break;
#if SYDBOX_HAVE_IPV6
                case AF_INET6:
                    if (data->addr->u.sa6.port[0] >= addr->u.sa6.port[0] &&
                            data->addr->u.sa6.port[1] <= addr->u.sa6.port[1])
                        violation = false;
                    break;
#endif /* SYDBOX_HAVE_IPV6 */
                default:
                    g_assert_not_reached();
            }

            if (!violation)
                break;
        }
    }

    if (violation) {
        switch (data->addr->family) {
            case AF_UNIX:
                sydbox_access_violation_net(child, data->addr, "%s{family=AF_UNIX path=%s abstract=%s}",
                        sname, data->addr->u.saun.sun_path,
                        data->addr->u.saun.abstract ? "true" : "false");
                break;
            case AF_INET:
                inet_ntop(AF_INET, &data->addr->u.sa.sin_addr, ip, sizeof(ip));
                sydbox_access_violation_net(child, data->addr, "%s{family=AF_INET addr=%s port=%d}",
                        sname, ip, data->addr->u.sa.port[0]);
                break;
#if SYDBOX_HAVE_IPV6
            case AF_INET6:
                inet_ntop(AF_INET6, &data->addr->u.sa6.sin6_addr, ip, sizeof(ip));
                sydbox_access_violation_net(child, data->addr, "%s{family=AF_INET6 addr=%s port=%d}",
                        sname, ip, data->addr->u.sa6.port[0]);
                break;
#endif /* SYDBOX_HAVE_IPV6 */
            default:
                g_assert_not_reached();
        }
        data->result = RS_DENY;
        /* For bind() set errno to EADDRNOTAVAIL.
         * For connect() and sendto() set errno to ECONNREFUSED.
         */
        child->retval = isbind ? -EADDRNOTAVAIL : -ECONNREFUSED;
    }
}

static void syscall_check(G_GNUC_UNUSED context_t *ctx, struct tchild *child, struct checkdata *data)
{
    if (G_UNLIKELY(RS_ALLOW != data->result))
        return;

    if (child->sandbox->network &&
            IS_NET_CALL(sflags) &&
            data->addr != NULL &&
            IS_SUPPORTED_FAMILY(data->addr->family)) {
        syscall_handle_net(child, data);
        return;
    }

    if (child->sandbox->exec && sflags & EXEC_CALL) {
        g_debug("checking `%s' for exec access", data->rpathlist[0]);
        if (G_UNLIKELY(!pathlist_check(child->sandbox->exec_prefixes, data->rpathlist[0]))) {
            sydbox_access_violation_exec(child, data->rpathlist[0],
                    "execve(\"%s\", [%s])", data->rpathlist[0], data->sargv);
            data->result = RS_DENY;
            child->retval = -EACCES;
        }
        return;
    }

    if (!child->sandbox->path)
        return;
    if (sflags & CHECK_PATH) {
        syscall_handle_path(child, data, 0);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
    if (sflags & CHECK_PATH2) {
        syscall_handle_path(child, data, 1);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
    if (sflags & CHECK_PATH_AT) {
        syscall_handle_path(child, data, 1);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
    if (sflags & CHECK_PATH_AT1) {
        syscall_handle_path(child, data, 2);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
    if (sflags & CHECK_PATH_AT2) {
        syscall_handle_path(child, data, 3);
        if (RS_ERROR == data->result || RS_DENY == data->result)
            return;
    }
}

static void syscall_check_finalize(G_GNUC_UNUSED context_t *ctx, struct tchild *child, struct checkdata *data)
{
    g_debug("ending check for system call %lu(%s), child %i", sno, sname, child->pid);

    for (unsigned int i = 0; i < 2; i++)
        g_free(data->dirfdlist[i]);
    for (unsigned int i = 0; i < 4; i++) {
        g_free(data->pathlist[i]);
        g_free(data->rpathlist[i]);
    }

    g_free(data->sargv);
    if (child->sandbox->network &&
            sydbox_config_get_network_auto_whitelist_bind() &&
            data->result == RS_ALLOW &&
            (sflags & BIND_CALL ||
             (sflags & DECODE_SOCKETCALL && data->subcall == PINK_SOCKET_SUBCALL_BIND)) &&
            data->addr != NULL &&
            IS_SUPPORTED_FAMILY(data->addr->family)) {
        /* Store the bind address.
         * We'll use it again to whitelist it when the system call is exiting.
         */
        child->bindlast = address_dup(data->addr);
    }
    address_free(data->addr);
}

/* Denied system call handler for system calls.
 * This function restores real call number for the denied system call and sets
 * return code.
 * Returns nonzero if child is dead, zero otherwise.
 */
static int syscall_handle_badcall(struct tchild *child)
{
    g_debug("restoring real call number for denied system call %lu(%s)", child->sno, sname);
    // Restore real call number and return our error code
    if (!pink_util_set_syscall(child->pid, child->bitness, child->sno)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error setting system call using ptrace()
             * child is still alive, hence the error is fatal.
             */
            g_critical("failed to restore system call: %s", g_strerror(errno));
            g_printerr("failed to restore system call: %s\n", g_strerror(errno));
            exit(-1);
        }
        // Child is dead.
        return -1;
    }
    if (!pink_util_set_return(child->pid, child->retval)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error setting return code using ptrace()
             * child is still alive, hence the error is fatal.
             */
            g_critical("failed to set return code: %s", g_strerror(errno));
            g_printerr("failed to set return code: %s\n", g_strerror(errno));
            exit(-1);
        }
        // Child is dead.
        return -1;
    }
    return 0;
}

/* chdir(2) handler for system calls.
 * This is only called when child is exiting chdir() or fchdir() system calls.
 * Returns nonzero if child is dead, zero otherwise.
 */
static int syscall_handle_chdir(struct tchild *child)
{
    long retval;

    if (!pink_util_get_return(child->pid, &retval)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error getting return code using ptrace()
             * child is still alive, hence the error is fatal.
             */
            g_critical("failed to get return code: %s", g_strerror(errno));
            g_printerr("failed to get return code: %s\n", g_strerror(errno));
            exit(-1);
        }
        // Child is dead.
        return -1;
    }
    if (0 == retval) {
        /* Child has successfully changed directory,
         * update current working directory.
         */
        char *newcwd = proc_getcwd(child->pid);
        if (NULL == newcwd) {
            /* Failed to get current working directory of child.
             * Set errno of the child.
             * FIXME: We should probably die here as well, because the
             * child has successfully changed directory and setting
             * errno doesn't change that fact.
             */
            retval = -errno;
            g_debug("proc_getcwd() failed: %s", g_strerror(errno));
            if (!pink_util_set_return(child->pid, retval)) {
                if (G_UNLIKELY(ESRCH != errno)) {
                    /* Error setting return code using ptrace()
                     * child is still alive, hence the error is fatal.
                     */
                    g_critical("failed to set return code: %s", g_strerror(errno));
                    g_printerr("failed to set return code: %s\n", g_strerror(errno));
                    exit(-1);
                }
                // Child is dead.
                return -1;
            }
        }
        else {
            /* Successfully determined the new current working
             * directory of child. Update context.
             */
            if (NULL != child->cwd)
                g_free(child->cwd);
            child->cwd = newcwd;
            g_info("child %i has changed directory to '%s'", child->pid, child->cwd);
        }
    }
    else {
        /* Child failed to change current working directory,
         * nothing to do.
         */
        g_debug("child %i failed to change directory: %s", child->pid, g_strerror(-retval));
    }
    return 0;
}

/**
 * bind(2) handler
 */
static int syscall_handle_bind(struct tchild *child, int flags)
{
    int port;
    long fd, retval, subcall;
    GSList *whitelist;

    if (!pink_util_get_return(child->pid, &retval)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error getting return code using ptrace()
             * child is still alive, hence the error is fatal.
             */
            g_critical("failed to get return code: %s", g_strerror(errno));
            g_printerr("failed to get return code: %s\n", g_strerror(errno));
            exit(-1);
        }
        // Child is dead.
        return -1;
    }

    if (0 != retval) {
        /* Bind call failed, ignore it */
        address_free(child->bindlast);
        child->bindlast = NULL;
        return 0;
    }

    if (flags & DECODE_SOCKETCALL) {
        if (!pink_util_get_arg(child->pid, child->bitness, 0, &subcall)) {
            if (G_UNLIKELY(ESRCH != errno)) {
                /* Error getting socket subcall using ptrace()
                 * child is still alive, hence the error is fatal.
                 */
                g_critical("Failed to decode socketcall: %s", g_strerror(errno));
                g_printerr("Failed to decode socketcall: %s\n", g_strerror(errno));
                exit(-1);
            }
            // Child is dead.
            return -1;
        }
        if (subcall != PINK_SOCKET_SUBCALL_BIND) {
            address_free(child->bindlast);
            child->bindlast = NULL;
            return 0;
        }
    }

    if (IS_SUPPORTED_FAMILY(child->bindlast->family)) {
        switch (child->bindlast->family) {
            case AF_UNIX:
                port = -1;
                break;
            case AF_INET:
                port = child->bindlast->u.sa.port[0];
                break;
#if SYDBOX_HAVE_IPV6
            case AF_INET6:
                port = child->bindlast->u.sa6.port[0];
                break;
#endif /* SYDBOX_HAVE_IPV6 */
            default:
                g_assert_not_reached();
        }

        if (port == 0) {
            /* Special case for binding to port zero.
             * We'll check the getsockname() call after this to get the port.
             */
            if (!pink_decode_socket_fd(child->pid, child->bitness, 0, &fd)) {
                if (G_UNLIKELY(ESRCH != errno)) {
                    /* Error getting return code using ptrace()
                     * child is still alive, hence the error is fatal.
                     */
                    g_critical("failed to get file descriptor: %s", g_strerror(errno));
                    g_printerr("failed to get file descriptor: %s\n", g_strerror(errno));
                    exit(-1);
                }
                // Child is dead.
                return -1;
            }
            g_hash_table_insert(child->bindzero, GINT_TO_POINTER(fd), address_dup(child->bindlast));
        }
        else {
            whitelist = sydbox_config_get_network_whitelist_connect();
            whitelist = g_slist_prepend(whitelist, address_dup(child->bindlast));
            sydbox_config_set_network_whitelist_connect(whitelist);
        }
    }

    address_free(child->bindlast);
    child->bindlast = NULL;
    return 0;
}

/**
 * getsockname(2) handler
 */
static int syscall_handle_getsockname(struct tchild *child, bool decode)
{
    long fd, retval, subcall;
    GSList *whitelist;
    struct sydbox_addr *addr, *addr_new;

    if (!pink_util_get_return(child->pid, &retval)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error getting return code using ptrace()
             * Silently ignore it.
             */
            g_debug("failed to get return code: %s", g_strerror(errno));
            return 0;
        }
        // Child is dead.
        return -1;
    }

    if (0 != retval) {
        g_debug("getsockname() call failed for child %i, ignoring", child->pid);
        return 0;
    }

    if (decode) { /* socketcall() */
        if (!pink_util_get_arg(child->pid, child->bitness, 0, &subcall)) {
            if (G_UNLIKELY(ESRCH != errno)) {
                /* Error getting socket subcall using ptrace()
                 * Silently ignore it.
                 */
                g_debug("failed to decode socketcall: %s", g_strerror(errno));
                return 0;
            }
            // Child is dead.
            return -1;
        }
        if (subcall != PINK_SOCKET_SUBCALL_GETSOCKNAME)
            return 0;

        addr_new = pinkw_get_socket_addr(child->pid, child->bitness, 1, &fd);
    }
    else /* getsockname() */
        addr_new = pinkw_get_socket_addr(child->pid, child->bitness, 1, &fd);

    if (addr_new == NULL) {
        /* Error getting fd using ptrace()
         * Silently ignore it.
         */
        g_debug("failed to get address: %s", g_strerror(errno));
        return 0;
    }

    addr = g_hash_table_lookup(child->bindzero, GINT_TO_POINTER(fd));
    if (addr == NULL) {
        g_debug("No bind() call received before getsockname(), ignoring");
        return 0;
    }

    switch (addr->family) {
        case AF_INET:
            addr->u.sa.port[0] = addr_new->u.sa.port[0];
            addr->u.sa.port[1] = addr_new->u.sa.port[0];
            g_debug("whitelisting last bind address with revealed bind port %d for connect",
                    addr->u.sa.port[0]);
            break;
#if SYDBOX_HAVE_IPV6
        case AF_INET6:
            addr->u.sa6.port[0] = addr_new->u.sa6.port[0];
            addr->u.sa6.port[1] = addr_new->u.sa6.port[1];
            g_debug("whitelisting last bind address with revealed bind port %d for connect",
                    addr->u.sa6.port[0]);
            break;
#endif /* SYDBOX_HAVE_IPV6 */
        default:
            g_assert_not_reached();
    }

    whitelist = sydbox_config_get_network_whitelist_connect();
    whitelist = g_slist_prepend(whitelist, address_dup(addr));
    sydbox_config_set_network_whitelist_connect(whitelist);

    g_free(addr_new);
    g_hash_table_remove(child->bindzero, GINT_TO_POINTER(fd));
    return 0;
}

/* dup() family handler
 */
static int syscall_handle_dup(struct tchild *child)
{
    long oldfd, newfd;
    struct sydbox_addr *addr;

    if (!pink_util_get_return(child->pid, &newfd)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error getting return code using ptrace()
             * Silently ignore it.
             */
            g_debug("failed to get new file descriptor: %s", g_strerror(errno));
            return 0;
        }
        // Child is dead.
        return -1;
    }

    if (0 > newfd) {
        /* Call failed, ignore it */
        return 0;
    }

    if (!pink_util_get_arg(child->pid, child->bitness, 0, &oldfd)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error getting first argument using ptrace()
             * Silently ignore it.
             */
            g_debug("failed to get old file descriptor: %s", g_strerror(errno));
            return 0;
        }
        // Child is dead.
        return -1;
    }

    addr = g_hash_table_lookup(child->bindzero, GINT_TO_POINTER(oldfd));
    if (addr == NULL) {
        g_debug("No bind() call received before dup() ignoring");
        return 0;
    }

    g_debug("Duplicating address information oldfd:%ld newfd:%ld", oldfd, newfd);
    g_hash_table_insert(child->bindzero, GINT_TO_POINTER(newfd), address_dup(addr));
    return 0;
}

static int syscall_handle_fcntl(struct tchild *child)
{
    long oldfd, newfd, cmd;
    struct sydbox_addr *addr;

    if (!pink_util_get_return(child->pid, &newfd)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error getting return code using ptrace()
             * Silently ignore it.
             */
            g_debug("failed to get return value of fcntl: %s", g_strerror(errno));
            return 0;
        }
        // Child is dead.
        return -1;
    }

    if (0 > newfd) {
        /* Call failed, ignore it */
        return 0;
    }

    if (!pink_util_get_arg(child->pid, child->bitness, 1, &cmd)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error getting first argument using ptrace()
             * Silently ignore it.
             */
            g_debug("failed to decode fcntl command: %s", g_strerror(errno));
            return 0;
        }
        // Child is dead.
        return -1;
    }

    if (F_DUPFD != cmd) {
        /* The command can't duplicate fd,
         * Ignore it.
         */
        return 0;
    }

    if (!pink_util_get_arg(child->pid, child->bitness, 0, &oldfd)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            /* Error getting first argument using ptrace()
             * Silently ignore it.
             */
            g_debug("failed to get fcntl fd: %s", g_strerror(errno));
            return 0;
        }
        // Child is dead.
        return -1;
    }

    addr = g_hash_table_lookup(child->bindzero, GINT_TO_POINTER(oldfd));
    if (addr == NULL) {
        g_debug("No bind() call received before fcntl() ignoring");
        return 0;
    }

    g_debug("Duplicating address information oldfd:%ld newfd:%ld", oldfd, newfd);
    g_hash_table_insert(child->bindzero, GINT_TO_POINTER(newfd), address_dup(addr));
    return 0;
}

/* Main syscall handler
 */
int syscall_handle(context_t *ctx, struct tchild *child)
{
    bool decode, entering;
    struct checkdata data;

    entering = !(child->flags & TCHILD_INSYSCALL);
    if (entering) {
        /* Child is entering the system call.
         * Get the system call number of child.
         * Save it in child->sno.
         */
        if (!pink_util_get_syscall(child->pid, child->bitness, &sno)) {
            if (G_UNLIKELY(ESRCH != errno)) {
                /* Error getting system call using ptrace()
                 * child is still alive, hence the error is fatal.
                 */
                g_critical("failed to get system call: %s", g_strerror(errno));
                g_printerr("failed to get system call: %s\n", g_strerror(errno));
                exit(-1);
            }
            // Child is dead, remove it
            return context_remove_child(ctx, child->pid);
        }
        child->sno = sno;
        sname = pink_name_syscall(sno, child->bitness);
        sflags = dispatch_lookup(sno, child->bitness);
    }
    else {
        sno = child->sno;
        sname = pink_name_syscall(sno, child->bitness);
        sflags = dispatch_lookup(sno, child->bitness);
    }

    if (entering) {
        g_debug_trace("child %i is entering system call %lu(%s)", child->pid, sno, sname);

        if (-1 == sflags) {
            /* No flags for this system call.
             * Safe system call, allow access.
             */
            g_debug_trace("allowing access to system call %lu(%s)", sno, sname);
        }
        else {
            memset(&data, 0, sizeof(struct checkdata));
            syscall_check_start(ctx, child, &data);
            syscall_check_flags(child, &data);
            syscall_check_magic(child, &data);
            syscall_check_resolve(child, &data);
            syscall_check_canonicalize(ctx, child, &data);
            syscall_check(ctx, child, &data);
            syscall_check_finalize(ctx, child, &data);

            /* Check result */
            switch(data.result) {
                case RS_ERROR:
                    errno = data.save_errno;
                    if (ESRCH == errno)
                        return context_remove_child(ctx, child->pid);
                    else if (EIO != errno && EFAULT != errno) {
                        g_critical("error while checking system call %lu(%s) for access: %s",
                                sno, sname, g_strerror(errno));
                        g_printerr("error while checking system call %lu(%s) for access: %s\n",
                                sno, sname, g_strerror(errno));
                        exit(-1);
                    }
                    else if (EIO == errno) {
                        /* Quoting from ptrace(2):
                         * There  was  an  attempt  to read from or write to an
                         * invalid area in the parent's or child's memory,
                         * probably because the area wasn't mapped or
                         * accessible. Unfortunately, under Linux, different
                         * variations of this fault will return EIO or EFAULT
                         * more or less arbitrarily.
                         */
                        /* For consistency we change the errno to EFAULT here.
                         * Because it's usually what we actually want.
                         * For example:
                         * open(NULL, O_RDONLY) (returns: -1, errno: EFAULT)
                         * under ptrace, we get errno: EIO
                         */
                        errno = EFAULT;
                    }
                    child->retval = -errno;
                    /* fall through */
                case RS_DENY:
                    g_debug("denying access to system call %lu(%s)", sno, sname);
                    child->flags |= TCHILD_DENYSYSCALL;
                    if (!pink_util_set_syscall(child->pid, child->bitness, PINK_SYSCALL_INVALID)) {
                        if (G_UNLIKELY(ESRCH != errno)) {
                            g_critical("failed to set system call: %s", g_strerror(errno));
                            g_printerr("failed to set system call: %s\n", g_strerror(errno));
                            exit(-1);
                        }
                        return context_remove_child(ctx, child->pid);
                    }
                    break;
                case RS_ALLOW:
                case RS_NOWRITE:
                case RS_MAGIC:
                    g_debug_trace("allowing access to system call %lu(%s)", sno, sname);
                    break;
                default:
                    g_assert_not_reached();
                    break;
            }
        }
    }
    else {
        g_debug_trace("child %i is exiting system call %lu(%s)", child->pid, sno, sname);

        if (child->flags & TCHILD_DENYSYSCALL) {
            /* Child is exiting a denied system call.
             */
            if (0 > syscall_handle_badcall(child))
                return context_remove_child(ctx, child->pid);
            child->flags &= ~TCHILD_DENYSYSCALL;
        }
        else if (dispatch_chdir(sno, child->bitness)) {
            /* Child is exiting a system call that may have changed its current
             * working directory. Update current working directory.
             */
            if (0 > syscall_handle_chdir(child))
                return context_remove_child(ctx, child->pid);
        }
        else if (child->sandbox->network && sydbox_config_get_network_auto_whitelist_bind()) {
            if (child->bindlast != NULL &&
                    (sflags & (DECODE_SOCKETCALL | BIND_CALL))) {
                if (0 > syscall_handle_bind(child, sflags))
                    return context_remove_child(ctx, child->pid);
            }
            if (g_hash_table_size(child->bindzero) > 0) {
                if (dispatch_maygetsockname(sno, child->bitness, &decode)) {
                    if (0 > syscall_handle_getsockname(child, decode))
                        return context_remove_child(ctx, child->pid);
                }
                else if (dispatch_dup(sno, child->bitness)) {
                    /* Child is exiting a system call that may have duplicated a file
                     * descriptor in child->bindzero. Update file descriptor
                     * information.
                     */
                    if (0 > syscall_handle_dup(child))
                        return context_remove_child(ctx, child->pid);
                }
                else if (dispatch_fcntl(sno, child->bitness)) {
                    /* Child is exiting a system call that may have duplicated a file
                     * descriptor in child->bindzero. Update file descriptor
                     * information.
                     */
                    if (0 > syscall_handle_fcntl(child))
                        return context_remove_child(ctx, child->pid);
                }
            }
        }
    }
    child->flags ^= TCHILD_INSYSCALL;
    return 0;
}

