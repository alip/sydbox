/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "log.h"
#include "path.h"
#include "children.h"

// We keep this for efficient lookups
struct tchild *childtab[PID_MAX_LIMIT] = { NULL };

void tchild_new(GSList **children, pid_t pid) {
    struct tchild *child, *parent;

    LOGD("New child %i", pid);
    child = (struct tchild *) g_malloc (sizeof(struct tchild));
    child->flags = TCHILD_NEEDSETUP;
    child->pid = pid;
    child->sno = 0xbadca11;
    child->retval = -1;
    child->cwd = NULL;
    child->sandbox = (struct tdata *) g_malloc (sizeof(struct tdata));
    child->sandbox->on = 1;
    child->sandbox->lock = LOCK_UNSET;
    child->sandbox->net = 1;
    child->sandbox->write_prefixes = NULL;
    child->sandbox->predict_prefixes = NULL;

    // Inheritance
    if (NULL != *children && NULL != (*children)->data) {
        parent = (*children)->data;

        if (NULL != parent->cwd) {
            LOGD("Child %i inherits parent %i's current working directory '%s'", pid, parent->pid, parent->cwd);
            child->cwd = g_strdup (parent->cwd);
        }

        if (NULL != parent->sandbox) {
            GSList *walk;
            child->sandbox->on = parent->sandbox->on;
            child->sandbox->lock = parent->sandbox->lock;
            child->sandbox->net = parent->sandbox->net;
            // Copy path lists
            walk = parent->sandbox->write_prefixes;
            while (NULL != walk) {
                pathnode_new(&(child->sandbox->write_prefixes), walk->data, 0);
                walk = g_slist_next(walk);
            }
            walk = parent->sandbox->predict_prefixes;
            while (NULL != walk) {
                pathnode_new(&(child->sandbox->predict_prefixes), walk->data, 0);
                walk = g_slist_next(walk);
            }
        }
    }
    childtab[pid] = child;
    *children = g_slist_prepend(*children, child);
}

static void tchild_free_one(struct tchild *child, void *user_data) {
    if (NULL != child->sandbox) {
        if (NULL != child->sandbox->write_prefixes)
            pathnode_free(&(child->sandbox->write_prefixes));
        if (NULL != child->sandbox->predict_prefixes)
            pathnode_free(&(child->sandbox->predict_prefixes));
        g_free (child->sandbox);
    }
    if (NULL != child->cwd)
        g_free (child->cwd);
    childtab[child->pid] = NULL;
    g_free (child);
}

void tchild_free(GSList **children) {
    LOGD("Freeing children %p", (void *) *children);
    g_slist_foreach(*children, (GFunc) tchild_free_one, NULL);
    g_slist_free(*children);
    *children = NULL;
}

void tchild_delete(GSList **children, pid_t pid) {
    GSList *walk;
    struct tchild *child;

    walk = *children;
    while (NULL != walk) {
        child = (struct tchild *) walk->data;
        if (child->pid == pid) {
            *children = g_slist_remove_link(*children, walk);
            tchild_free_one(walk->data, NULL);
            g_slist_free(walk);
            break;
        }
        walk = g_slist_next(walk);
    }
}
