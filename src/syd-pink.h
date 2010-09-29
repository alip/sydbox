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

#ifndef SYDBOX_GUARD_SYD_PINK_H
#define SYDBOX_GUARD_SYD_PINK_H 1

#include <stdbool.h>
#include <sys/types.h>
#include <pinktrace/pink.h>

#include "syd-net.h"

bool pinkw_trace_setup_all(pid_t pid);
bool pinkw_trace_resume(pid_t pid);
bool pinkw_encode_stat(pid_t pid, pink_bitness_t bitness);
struct sydbox_addr *pinkw_get_socket_addr(pid_t pid, pink_bitness_t bitness, unsigned ind, long *fd);
char *pinkw_stringify_argv(pid_t pid, pink_bitness_t bitness, unsigned ind);

#endif // SYDBOX_GUARD_SYD_PINK_H

