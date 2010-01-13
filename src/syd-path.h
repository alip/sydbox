/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Saleem Abdulrasool <compnerd@compnerd.org>
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

#ifndef SYDBOX_GUARD_PATH_H
#define SYDBOX_GUARD_PATH_H 1

#include <stdbool.h>

#include <glib.h>

#define CMD_PATH                        "/dev/sydbox/"
#define CMD_ON                          CMD_PATH"on"
#define CMD_OFF                         CMD_PATH"off"
#define CMD_TOGGLE                      CMD_PATH"toggle"
#define CMD_ENABLED                     CMD_PATH"enabled"
#define CMD_LOCK                        CMD_PATH"lock"
#define CMD_EXEC_LOCK                   CMD_PATH"exec_lock"
#define CMD_WAIT_ALL                    CMD_PATH"wait/all"
#define CMD_WAIT_ELDEST                 CMD_PATH"wait/eldest"
#define CMD_WRAP_LSTAT                  CMD_PATH"wrap/lstat"
#define CMD_NOWRAP_LSTAT                CMD_PATH"nowrap/lstat"
#define CMD_WRITE                       CMD_PATH"write/"
#define CMD_RMWRITE                     CMD_PATH"unwrite/"
#define CMD_SANDBOX_EXEC                CMD_PATH"sandbox/exec"
#define CMD_SANDUNBOX_EXEC              CMD_PATH"sandunbox/exec"
#define CMD_ADDEXEC                     CMD_PATH"addexec/"
#define CMD_RMEXEC                      CMD_PATH"rmexec/"
#define CMD_SANDBOX_NET                 CMD_PATH"sandbox/net"
#define CMD_SANDUNBOX_NET               CMD_PATH"sandunbox/net"
#define CMD_ADDFILTER                   CMD_PATH"addfilter/"
#define CMD_RMFILTER                    CMD_PATH"rmfilter/"
#define CMD_NET_WHITELIST_BIND          CMD_PATH"net/whitelist/bind/"
#define CMD_NET_UNWHITELIST_BIND        CMD_PATH"net/unwhitelist/bind/"
#define CMD_NET_WHITELIST_CONNECT       CMD_PATH"net/whitelist/connect/"
#define CMD_NET_UNWHITELIST_CONNECT     CMD_PATH"net/unwhitelist/connect/"

bool path_magic_dir(const char *path);

bool path_magic_on(const char *path);

bool path_magic_off(const char *path);

bool path_magic_toggle(const char *path);

bool path_magic_enabled(const char *path);

bool path_magic_lock(const char *path);

bool path_magic_exec_lock(const char *path);

bool path_magic_wait_all(const char *path);

bool path_magic_wait_eldest(const char *path);

bool path_magic_write(const char *path);

bool path_magic_rmwrite(const char *path);

bool path_magic_wrap_lstat(const char *path);

bool path_magic_nowrap_lstat(const char *path);

bool path_magic_sandbox_exec(const char *path);

bool path_magic_sandunbox_exec(const char *path);

bool path_magic_addexec(const char *path);

bool path_magic_rmexec(const char *path);

bool path_magic_sandbox_net(const char *path);

bool path_magic_sandunbox_net(const char *path);

bool path_magic_addfilter(const char *path);

bool path_magic_rmfilter(const char *path);

bool path_magic_net_allow(const char *path);

bool path_magic_net_deny(const char *path);

bool path_magic_net_local(const char *path);

bool path_magic_net_whitelist_bind(const char *path);

bool path_magic_net_unwhitelist_bind(const char *path);

bool path_magic_net_whitelist_connect(const char *path);

bool path_magic_net_unwhitelist_connect(const char *path);

int pathnode_new(GSList **pathlist, const char *path, int sanitize);

int pathnode_new_early(GSList **pathlist, const char *path, int sanitize);

void pathnode_free(GSList **pathlist);

void pathnode_delete(GSList **pathlist, const char *path_sanitized);

int pathlist_init(GSList **pathlist, const char *pathlist_env);

int pathlist_check(GSList *pathlist, const char *path_sanitized);

#endif // SYDBOX_GUARD_PATH_H

