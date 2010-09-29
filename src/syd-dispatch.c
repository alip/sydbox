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
#endif // HAVE_CONFIG_H

#include <stdbool.h>
#include <asm/unistd.h>

#include <glib.h>
#include <pinktrace/pink.h>

#include "syd-dispatch.h"
#include "syd-dispatch-table.h"

static GHashTable *flags = NULL;

void dispatch_init(void)
{
    if (flags == NULL) {
        flags = g_hash_table_new(g_direct_hash, g_direct_equal);
        for (unsigned int i = 0; -1 != syscalls[i].no; i++)
            g_hash_table_insert(flags, GINT_TO_POINTER(syscalls[i].no), GINT_TO_POINTER(syscalls[i].flags));
    }
}

void dispatch_free(void)
{
    if (flags != NULL) {
        g_hash_table_destroy(flags);
        flags = NULL;
    }
}

int dispatch_lookup(int sno, G_GNUC_UNUSED pink_bitness_t bitness)
{
    gpointer f;

    g_assert(flags != NULL);
    f = g_hash_table_lookup(flags, GINT_TO_POINTER(sno));
    return (f == NULL) ? -1 : GPOINTER_TO_INT(f);
}

inline
bool dispatch_chdir(int sno, G_GNUC_UNUSED pink_bitness_t bitness)
{
    return (__NR_chdir == sno) || (__NR_fchdir == sno);
}

inline
bool dispatch_dup(int sno, G_GNUC_UNUSED pink_bitness_t bitness)
{
#if defined(__NR_dup3)
    return (__NR_dup == sno) || (__NR_dup2 == sno) || (__NR_dup3 == sno);
#else
    return (__NR_dup == sno) || (__NR_dup2 == sno);
#endif
}

inline
bool dispatch_fcntl(int sno, G_GNUC_UNUSED pink_bitness_t bitness)
{
#if defined(__NR_fcntl64)
    return (__NR_fcntl == sno) || (__NR_fcntl64 == sno);
#else
    return (__NR_fcntl == sno);
#endif
}

inline
bool dispatch_maygetsockname(int sno, G_GNUC_UNUSED pink_bitness_t bitness, bool *decode)
{
#if defined(__NR_socketcall)
    if (__NR_socketcall == sno) {
        if (decode != NULL)
            *decode = true;
        return true;
    }
    return false;
#elif defined(__NR_getsockname)
    if (__NR_getsockname == sno) {
        if (decode != NULL)
            *decode = false;
        return true;
    }
    return false;
#else
#error wtf? neither socketcall() nor getsockname() defined
#endif
}

