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

#ifndef SYDBOX_GUARD_UTIL_H
#define SYDBOX_GUARD_UTIL_H 1

#include <sys/types.h>

int upeek(pid_t pid, long off, long *res);
int umoven(pid_t pid, long addr, char *dest, size_t len);
int umovestr(pid_t pid, long addr, char *dest, size_t len);

#define umove(pid, addr, objp)  \
    umoven((pid), (addr), (char *)(objp), sizeof *(objp))

#endif // SYDBOX_GUARD_UTIL_H
