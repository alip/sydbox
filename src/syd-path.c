/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
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

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "syd-log.h"
#include "syd-path.h"
#include "syd-utils.h"

static char *shell_expand(const char * const str)
{
    gchar *argv[4] = { "/bin/sh", "-c", NULL, NULL };
    gchar *quoted, *output = NULL;
    GError *spawn_error = NULL;
    gint retval, nlindex;

    quoted = g_shell_quote(str);
    argv[2] = g_strdup_printf("echo '%s'", quoted);
    g_free(quoted);

    if (G_UNLIKELY(!g_spawn_sync(NULL, argv, NULL, G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL,
                    &output, NULL, &retval, &spawn_error))) {
        g_printerr("failed to expand `%s': %s\n", str, spawn_error->message);
        g_error_free(spawn_error);
    }

    g_free(argv[2]);

    // Lose the newline at the end.
    nlindex = strlen(output) - 1;
    g_assert('\n' == output[nlindex]);
    output[nlindex] = '\0';
    return output;
}

inline bool path_magic_dir(const char *path)
{
    return (0 == strncmp(path, CMD_PATH, sizeof(CMD_PATH) - 2));
}

inline bool path_magic_on(const char *path)
{
    return (0 == strncmp(path, CMD_ON, sizeof(CMD_ON)));
}

inline bool path_magic_off(const char *path)
{
    return  (0 == strncmp(path, CMD_OFF, sizeof(CMD_OFF)));
}

inline bool path_magic_toggle(const char *path)
{
    return (0 == strncmp(path, CMD_TOGGLE, sizeof(CMD_TOGGLE)));
}

inline bool path_magic_enabled(const char *path)
{
    return (0 == strncmp(path, CMD_ENABLED, sizeof(CMD_ENABLED)));
}

inline bool path_magic_lock(const char *path)
{
    return (0 == strncmp(path, CMD_LOCK, sizeof(CMD_LOCK)));
}

inline bool path_magic_exec_lock(const char *path)
{
    return (0 == strncmp(path, CMD_EXEC_LOCK, sizeof(CMD_EXEC_LOCK)));
}

inline bool path_magic_wait_all(const char *path)
{
    return (0 == strncmp(path, CMD_WAIT_ALL, sizeof(CMD_WAIT_ALL)));
}

inline bool path_magic_wait_eldest(const char *path)
{
    return (0 == strncmp(path, CMD_WAIT_ELDEST, sizeof(CMD_WAIT_ELDEST)));
}

inline bool path_magic_wrap_lstat(const char *path)
{
    return (0 == strncmp(path, CMD_WRAP_LSTAT, sizeof(CMD_WRAP_LSTAT)));
}

inline bool path_magic_nowrap_lstat(const char *path)
{
    return (0 == strncmp(path, CMD_NOWRAP_LSTAT, sizeof(CMD_NOWRAP_LSTAT)));
}

inline bool path_magic_write(const char *path)
{
    return (0 == strncmp(path, CMD_WRITE, sizeof(CMD_WRITE) - 1));
}

inline bool path_magic_rmwrite(const char *path)
{
    return (0 == strncmp(path, CMD_RMWRITE, sizeof(CMD_RMWRITE) - 1));
}

inline bool path_magic_sandbox_exec(const char *path)
{
    return (0 == strncmp(path, CMD_SANDBOX_EXEC, sizeof(CMD_SANDBOX_EXEC)));
}

inline bool path_magic_sandunbox_exec(const char *path)
{
    return (0 == strncmp(path, CMD_SANDUNBOX_EXEC, sizeof(CMD_SANDUNBOX_EXEC)));
}

inline bool path_magic_addexec(const char *path)
{
    return (0 == strncmp(path, CMD_ADDEXEC, sizeof(CMD_ADDEXEC) - 1));
}

inline bool path_magic_rmexec(const char *path)
{
    return (0 == strncmp(path, CMD_RMEXEC, sizeof(CMD_RMEXEC) - 1));
}

inline bool path_magic_sandbox_net(const char *path)
{
    return (0 == strncmp(path, CMD_SANDBOX_NET, sizeof(CMD_SANDBOX_NET)));
}

inline bool path_magic_sandunbox_net(const char *path)
{
    return (0 == strncmp(path, CMD_SANDUNBOX_NET, sizeof(CMD_SANDUNBOX_NET)));
}

inline bool path_magic_addfilter(const char *path)
{
    return (0 == strncmp(path, CMD_ADDFILTER, sizeof(CMD_ADDFILTER) - 1));
}

inline bool path_magic_rmfilter(const char *path)
{
    return (0 == strncmp(path, CMD_RMFILTER, sizeof(CMD_RMFILTER) - 1));
}

inline bool path_magic_addfilter_exec(const char *path)
{
    return (0 == strncmp(path, CMD_ADDFILTER_EXEC, sizeof(CMD_ADDFILTER_EXEC) - 1));
}

inline bool path_magic_rmfilter_exec(const char *path)
{
    return (0 == strncmp(path, CMD_RMFILTER_EXEC, sizeof(CMD_RMFILTER_EXEC) - 1));
}

inline bool path_magic_addfilter_net(const char *path)
{
    return (0 == strncmp(path, CMD_ADDFILTER_NET, sizeof(CMD_ADDFILTER_NET) - 1));
}

inline bool path_magic_rmfilter_net(const char *path)
{
    return (0 == strncmp(path, CMD_RMFILTER_NET, sizeof(CMD_RMFILTER_NET) - 1));
}

inline bool path_magic_net_whitelist_bind(const char *path)
{
    return (0 == strncmp(path, CMD_NET_WHITELIST_BIND, sizeof(CMD_NET_WHITELIST_BIND) - 1));
}

inline bool path_magic_net_unwhitelist_bind(const char *path)
{
    return (0 == strncmp(path, CMD_NET_UNWHITELIST_BIND, sizeof(CMD_NET_UNWHITELIST_BIND) - 1));
}

inline bool path_magic_net_whitelist_connect(const char *path)
{
    return (0 == strncmp(path, CMD_NET_WHITELIST_CONNECT, sizeof(CMD_NET_WHITELIST_CONNECT) - 1));
}

inline bool path_magic_net_unwhitelist_connect(const char *path)
{
    return (0 == strncmp(path, CMD_NET_UNWHITELIST_CONNECT, sizeof(CMD_NET_UNWHITELIST_CONNECT) - 1));
}

int pathnode_new(GSList **pathlist, const char *path, int sanitize)
{
    char *data;

    if (G_UNLIKELY(NULL == path)) {
        g_info("path is NULL not adding to list");
        return -1;
    }
    else if (G_UNLIKELY('\0' == path[0])) {
        g_info("path is empty not adding to list");
        return -1;
    }

    if (G_LIKELY(!sanitize))
        data = g_strdup(path);
    else {
        char *spath = sydbox_compress_path(path);
        data = shell_expand(spath);
        g_free(spath);
        /* shell_expand() may return empty string! */
        if (G_UNLIKELY('\0' == data[0])) {
            g_info("shell_expand() returned empty string for `%s', not adding to list", path);
            g_free(data);
            return -1;
        }
        else
            g_info("new path item `%s'", data);
    }
    *pathlist = g_slist_prepend(*pathlist, data);
    return 0;
}

int pathnode_new_early(GSList **pathlist, const char *path, int sanitize)
{
    char *data, *spath;

    if (G_UNLIKELY(NULL == path || '\0' == path[0]))
        return -1;

    if (!sanitize)
        data = g_strdup(path);
    else {
        spath = sydbox_compress_path(path);
        data = shell_expand(spath);
        g_free(spath);
        /* shell_expand() may return empty string! */
        if (G_UNLIKELY('\0' == data[0])) {
            g_free(data);
            return -1;
        }
    }
    *pathlist = g_slist_prepend(*pathlist, data);
    return 0;
}

void pathnode_free(GSList **pathlist)
{
    g_slist_foreach(*pathlist, (GFunc) g_free, NULL);
    g_slist_free(*pathlist);
    *pathlist = NULL;
}

void pathnode_delete(GSList **pathlist, const char *path_sanitized)
{
    GSList *walk;

    for (walk = *pathlist; walk != NULL; walk = g_slist_next(walk)) {
        if (0 == strncmp(walk->data, path_sanitized, strlen(path_sanitized) + 1)) {
            g_debug("freeing pathnode %p", (void *) walk);
            *pathlist = g_slist_remove_link(*pathlist, walk);
            g_free(walk->data);
            g_slist_free(walk);
            break;
        }
    }
}

int pathlist_init(GSList **pathlist, const char *pathlist_env)
{
    char **split;
    int nempty, npaths;

    if (NULL == pathlist_env) {
        g_info("the given environment variable isn't set");
        return 0;
    }

    nempty = 0;
    split = g_strsplit(pathlist_env, ":", -1);
    for (unsigned int i = 0; i < g_strv_length(split); i++) {
        if (0 != strncmp(split[i], "", 2))
            *pathlist = g_slist_prepend(*pathlist, g_strdup(split[i]));
        else {
            g_debug("ignoring empty path element in position %d", i);
            ++nempty;
        }
    }
    npaths = g_strv_length(split) - nempty;
    g_strfreev(split);
    g_info("initialized path list with %d paths", npaths);
    return npaths;
}

int pathlist_check(GSList *pathlist, const char *path_sanitized)
{
    int ret;
    GSList *walk;

    g_debug("checking `%s'", path_sanitized);

    ret = 0;
    for (walk = pathlist; walk != NULL; walk = g_slist_next(walk)) {
        if (0 == strncmp(walk->data, "/", 2)) {
            g_debug("`%s' begins with `%s'", path_sanitized, (char *) walk->data);
            ret = 1;
            break;
        }
        else if (0 == strncmp(path_sanitized, walk->data, strlen(walk->data))) {
            if (strlen(path_sanitized) > strlen(walk->data)) {
                /* Path begins with one of the allowed paths. Check for a
                 * zero byte or a / on the next character so that for example
                 * /devzero/foo doesn't pass the test when /dev is the only
                 * allowed path.
                 */
                const char last = path_sanitized[strlen(walk->data)];
                if ('\0' == last || '/' == last) {
                    g_debug("`%s' begins with `%s'", path_sanitized, (char *) walk->data);
                    ret = 1;
                    break;
                }
                else
                    g_debug("`%s' doesn't begin with `%s'", path_sanitized, (char *) walk->data);
            }
            else {
                g_debug("`%s' begins with `%s'", path_sanitized, (char *) walk->data);
                ret = 1;
                break;
            }
        }
        else
            g_debug("`%s' doesn't begin with `%s'", path_sanitized, (char *) walk->data);
    }
    if (ret)
        g_debug("path list check succeeded for `%s'", path_sanitized);
    else
        g_debug("path list check failed for `%s'", path_sanitized);
    return ret;
}

