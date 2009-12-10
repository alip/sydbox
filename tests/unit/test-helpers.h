/* vim: set et ts=4 sts=4 sw=4 fdm=syntax : */

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

#include <glib.h>

#define XFAIL(...)                  \
    do {                            \
        g_printerr(__VA_ARGS__);    \
        g_assert(FALSE);            \
    } while (0)

#define XFAIL_IF(expr, ...)     \
    do {                        \
        if (expr) {             \
            XFAIL(__VA_ARGS__); \
        }                       \
    } while (0)

#define XFAIL_UNLESS(expr, ...) \
    do {                        \
        if (!(expr)) {          \
            XFAIL(__VA_ARGS__); \
        }                       \
    } while (0)

