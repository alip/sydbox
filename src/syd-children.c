/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009, 2010 Ali Polatel <alip@exherbo.org>
 * Based in part upon catbox which is:
 *  Copyright (c) 2006-2007 TUBITAK/UEKAE
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

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <pinktrace/pink.h>

#include "syd-children.h"
#include "syd-config.h"
#include "syd-log.h"
#include "syd-path.h"
#include "syd-pink.h"
#include "syd-net.h"

struct tchild *tchild_new(GHashTable *children, pid_t pid, bool eldest)
{
    char proc_pid[32];
    struct tchild *child;

    g_debug("new child %i", pid);
    child = g_new(struct tchild, 1);
    child->flags = eldest ? TCHILD_NEEDSETUP : TCHILD_NEEDSETUP | TCHILD_NEEDINHERIT;
    child->pid = pid;
    child->sno = 0xbadca11;
    child->retval = -1;
    child->cwd = NULL;
    child->lastexec = g_string_new("");
    child->bindzero = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    child->bindlast = NULL;
    child->sandbox = g_new(struct tdata, 1);
    child->sandbox->path = true;
    child->sandbox->exec = false;
    child->sandbox->network = false;
    child->sandbox->lock = LOCK_UNSET;
    child->sandbox->write_prefixes = NULL;
    child->sandbox->exec_prefixes = NULL;

    if (!eldest && sydbox_config_get_allow_proc_pid()) {
        /* Allow /proc/%i which is needed for processes to work reliably. */
        snprintf(proc_pid, 32, "/proc/%i", pid);
        pathnode_new(&(child->sandbox->write_prefixes), proc_pid, false);
    }

    g_hash_table_insert(children, GINT_TO_POINTER(pid), child);
    return child;
}

void tchild_inherit(struct tchild *child, struct tchild *parent)
{
    GSList *walk;

    g_assert(NULL != child && NULL != parent);
    if (!(child->flags & TCHILD_NEEDINHERIT))
        return;

    if (G_LIKELY(NULL != parent->cwd)) {
        g_debug("child %i inherits parent %i's current working directory `%s'", child->pid, parent->pid, parent->cwd);
        child->cwd = g_strdup(parent->cwd);
    }

    child->lastexec = g_string_assign(child->lastexec, parent->lastexec->str);
    child->bitness = parent->bitness;
    child->sandbox->path = parent->sandbox->path;
    child->sandbox->exec = parent->sandbox->exec;
    child->sandbox->network = parent->sandbox->network;
    child->sandbox->lock = parent->sandbox->lock;
    // Copy path lists
    for (walk = parent->sandbox->write_prefixes; walk != NULL; walk = g_slist_next(walk))
        pathnode_new(&(child->sandbox->write_prefixes), walk->data, false);
    for (walk = parent->sandbox->exec_prefixes; walk != NULL; walk = g_slist_next(walk))
        pathnode_new(&(child->sandbox->exec_prefixes), walk->data, false);
    child->flags &= ~TCHILD_NEEDINHERIT;
}

void tchild_free_one(gpointer child_ptr)
{
    struct tchild *child = (struct tchild *) child_ptr;

    if (G_LIKELY(NULL != child->sandbox)) {
        if (G_LIKELY(NULL != child->sandbox->write_prefixes))
            pathnode_free(&(child->sandbox->write_prefixes));
        if (G_LIKELY(NULL != child->sandbox->exec_prefixes))
            pathnode_free(&(child->sandbox->exec_prefixes));
        g_free(child->sandbox);
    }
    if (G_LIKELY(NULL != child->lastexec))
        g_string_free(child->lastexec, TRUE);
    if (G_LIKELY(NULL != child->bindzero))
        g_hash_table_destroy(child->bindzero);
    address_free(child->bindlast);
    g_free(child->cwd);
    g_free(child);
}

void tchild_kill_one(gpointer pid_ptr, G_GNUC_UNUSED gpointer child_ptr, G_GNUC_UNUSED void *userdata)
{
    pink_trace_kill(GPOINTER_TO_INT(pid_ptr));
}

void tchild_resume_one(gpointer pid_ptr, G_GNUC_UNUSED gpointer child_ptr, G_GNUC_UNUSED void *userdata)
{
    pinkw_trace_resume(GPOINTER_TO_INT(pid_ptr));
}

void tchild_delete(GHashTable *children, pid_t pid)
{
    g_hash_table_remove(children, GINT_TO_POINTER(pid));
}

struct tchild *tchild_find(GHashTable *children, pid_t pid)
{
    return g_hash_table_lookup(children, GINT_TO_POINTER(pid));
}

