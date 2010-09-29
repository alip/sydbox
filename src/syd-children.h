/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Saleem Abdulrasool <compnerd@compnerd.org>
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

#ifndef SYDBOX_GUARD_CHILDREN_H
#define SYDBOX_GUARD_CHILDREN_H

#include <stdbool.h>
#include <sys/types.h>

#include <glib.h>
#include <pinktrace/pink.h>

#include "syd-net.h"

/* TCHILD flags */
#define TCHILD_NEEDSETUP   (1 << 0)    /* child needs setup. */
#define TCHILD_NEEDINHERIT (1 << 1)    /* child needs to inherit sandbox data from her parent. */
#define TCHILD_INSYSCALL   (1 << 2)    /* child is in syscall. */
#define TCHILD_DENYSYSCALL (1 << 3)    /* child has been denied access to the syscall. */

/* per process tracking data */
enum lock_status
{
    LOCK_SET,        // Magic commands are locked.
    LOCK_UNSET,      // Magic commands are unlocked.
    LOCK_PENDING,    // Magic commands will be locked when an execve() is encountered.
};

struct tdata
{
    bool path;              // Whether path sandboxing is enabled for child.
    bool exec;              // Whether execve(2) sandboxing is enabled for child.
    bool network;           // Whether network sandboxing is enabled for child.
    int lock;               // Whether magic commands are locked for the child.
    GSList *write_prefixes;
    GSList *exec_prefixes;
};

struct tchild
{
    int flags;               // TCHILD_ flags
    pid_t pid;               // Process ID of the child.
    pink_bitness_t bitness;  // Bitness (32bit, 64bit etc.)
    char *cwd;               // Child's current working directory.
    unsigned long sno;       // Last system call called by child.
    long retval;             // Replaced system call will return this value.
    GString *lastexec;       // Last execve() arguments converted to string (used for debugging)
    GHashTable *bindzero;    // List of addresses whose port argument was zero.
    struct sydbox_addr *bindlast; // Last bind() address
    struct tdata *sandbox;   // Sandbox data
};

struct tchild *tchild_new(GHashTable *children, pid_t pid, bool eldest);

void tchild_inherit(struct tchild *child, struct tchild *parent);

void tchild_free_one(gpointer child_ptr);

void tchild_kill_one(gpointer pid_ptr, gpointer child_ptr, void *userdata);

void tchild_resume_one(gpointer pid_ptr, gpointer child_ptr, void *userdata);

void tchild_delete(GHashTable *children, pid_t pid);

struct tchild *tchild_find(GHashTable *children, pid_t pid);

#endif // SYDBOX_GUARD_CHILDREN_H

