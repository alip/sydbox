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

#ifndef SYDBOX_GUARD_SYSCALL_H
#define SYDBOX_GUARD_SYSCALL_H 1

#include <stdbool.h>
#include <sysexits.h>

#include <glib.h>

#include "children.h"
#include "context.h"

enum {
    RS_ALLOW,
    RS_NOWRITE,
    RS_MAGIC,
    RS_DENY,
    RS_ERROR = EX_SOFTWARE
};

struct checkdata {
    gint result;            // Check result
    gint save_errno;        // errno when the result is RS_ERROR

    bool resolve;           // true if the system call resolves paths
    glong open_flags;       // flags argument of open()/openat()
    glong access_flags;     // flags argument of access()/faccessat()
    gchar *dirfdlist[2];    // dirfd arguments (resolved)
    gchar *pathlist[4];     // Path arguments
    gchar *rpathlist[4];    // Path arguments (canonicalized)

    int socket_subcall;     // Socketcall() subcall
    int family;             // Family of destination address (AF_UNIX, AF_INET etc.)
    int port;               // Port of destination address
    gchar *addr;            // Destination address for socket calls
};

int syscall_handle(context_t *ctx, struct tchild *child);

#endif // SYDBOX_GUARD_SYSCALL_H

