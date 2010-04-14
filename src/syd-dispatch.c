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

#include "syd-flags.h"
#include "syd-dispatch.h"
#include "syd-dispatch-table.h"

static const struct syscall_name {
    int no;
    const char *name;
} sysnames[] = {
#include "syd-syscallent.h"
    {-1,    NULL}
};

static GHashTable *flags = NULL;
static GHashTable *names = NULL;

void dispatch_init(void)
{
    if (flags == NULL) {
        flags = g_hash_table_new(g_direct_hash, g_direct_equal);
        for (unsigned int i = 0; -1 != syscalls[i].no; i++)
            g_hash_table_insert(flags, GINT_TO_POINTER(syscalls[i].no), GINT_TO_POINTER(syscalls[i].flags));
    }
    if (names == NULL) {
        names = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
        for (unsigned int i = 0; NULL != sysnames[i].name; i++)
            g_hash_table_insert(names, GINT_TO_POINTER(sysnames[i].no), g_strdup(sysnames[i].name));
    }
}

void dispatch_free(void)
{
    if (flags != NULL) {
        g_hash_table_destroy(flags);
        flags = NULL;
    }
    if (names != NULL) {
        g_hash_table_destroy(names);
        names = NULL;
    }
}

int dispatch_lookup(G_GNUC_UNUSED int personality, int sno)
{
    gpointer f;

    g_assert(flags != NULL);
    f = g_hash_table_lookup(flags, GINT_TO_POINTER(sno));
    return (f == NULL) ? -1 : GPOINTER_TO_INT(f);
}

const char *dispatch_name(G_GNUC_UNUSED int personality, int sno)
{
    const char *sname;

    g_assert(names != NULL);
    sname = (const char *) g_hash_table_lookup(names, GINT_TO_POINTER(sno));
    return sname ? sname : UNKNOWN_SYSCALL;
}

inline const char *dispatch_mode(G_GNUC_UNUSED int personality)
{
    const char *mode;
#if defined(I386)
    mode = "32 bit";
#elif defined(IA64)
    mode = "64 bit";
#elif defined(POWERPC64)
    mode = "64 bit";
#elif defined(POWERPC)
    mode = "32 bit";
#else
#error unsupported architecture
#endif
    return mode;
}

inline bool dispatch_chdir(G_GNUC_UNUSED int personality, int sno)
{
    return IS_CHDIR(sno);
}

