/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009, 2010, 2011 Ali Polatel <alip@exherbo.org>
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
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <glib.h>
#include <pinktrace/pink.h>

#include "syd-children.h"
#include "syd-config.h"
#include "syd-log.h"
#include "syd-loop.h"
#include "syd-pink.h"
#include "syd-proc.h"
#include "syd-syscall.h"

// Event handlers
static int event_setup(context_t *ctx, struct tchild *child)
{
    if (!pinkw_trace_setup_all(child->pid)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to set tracing options: %s", g_strerror(errno));
            g_printerr("failed to set tracing options: %s\n", g_strerror(errno));
            exit(-1);
        }
        return context_remove_child(ctx, child->pid);
    }
    else
        child->flags &= ~TCHILD_NEEDSETUP;
    return 0;
}

static int event_syscall(context_t *ctx, struct tchild *child)
{
    if (!pink_trace_syscall(child->pid, 0)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to resume child %i: %s", child->pid, g_strerror(errno));
            g_printerr("failed to resume child %i: %s\n", child->pid, g_strerror(errno));
            exit(-1);
        }
        return context_remove_child(ctx, child->pid);
    }
    return 0;
}

static int event_fork(context_t *ctx, struct tchild *child)
{
    unsigned long childpid;
    struct tchild *newchild;

    // Get new child's pid
    if (G_UNLIKELY(!pink_trace_geteventmsg(child->pid, &childpid))) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to get the pid of the newborn child: %s", g_strerror(errno));
            g_printerr("failed to get the pid of the newborn child: %s\n", g_strerror(errno));
            exit(-1);
        }
        return context_remove_child(ctx, child->pid);
    }
    else
        g_debug("the newborn child's pid is %ld", childpid);

    newchild = tchild_find(ctx->children, childpid);
    if (NULL == newchild) {
        /* Child hasn't been born yet, add it to the list of children and
         * inherit parent's sandbox data.
         */
        newchild = tchild_new(ctx->children, childpid, false);
        tchild_inherit(newchild, child);
    }
    else if (newchild->flags & TCHILD_NEEDINHERIT) {
        /* Child has already been born but hasn't inherited parent's sandbox data
         * yet. Inherit parent's sandbox data and resume the child.
         */
        g_debug("prematurely born child %i inherits sandbox data from her parent %i", newchild->pid, child->pid);
        tchild_inherit(newchild, child);
        event_syscall(ctx, newchild);
    }
    return 0;
}

static int event_exit(context_t *ctx, pid_t pid, int *code_ptr)
{
    int code;
    unsigned long status;

    if (pid != ctx->eldest)
        goto resume;

    // Get the exit status
    if (G_UNLIKELY(!pink_trace_geteventmsg(pid, &status))) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to get the exit status of the dying child: %s", g_strerror(errno));
            g_printerr("failed to get the exit status of the dying child: %s\n", g_strerror(errno));
            exit(-1);
        }
    }

    if (WIFEXITED(status)) {
        code = WEXITSTATUS(status);
        g_info("Eldest child %i is exiting with status:%#lx", pid, status);
    }
    else if (WIFSIGNALED(status)) {
        code = 128 + WTERMSIG(status);
        g_info("Eldest child %i is terminating with status:%#lx", pid, status);
    }
    else
        g_assert_not_reached();

    if (code_ptr)
        *code_ptr = code;

    if (!sydbox_config_get_wait_all()) {
        g_hash_table_foreach(ctx->children, tchild_resume_one, NULL);
        g_hash_table_destroy(ctx->children);
        ctx->children = NULL;
        return -1;
    }

resume:
    if (G_UNLIKELY(!pink_trace_detach(pid, 0))) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to detach the dying child %i: %s", pid, g_strerror(errno));
            g_printerr("failed to detach the dying child %i: %s\n", pid, g_strerror(errno));
            exit(-1);
        }
    }

    tchild_delete(ctx->children, pid);
    return 0;
}

static int event_genuine(context_t * ctx, struct tchild *child, int status)
{
    int sig = WSTOPSIG(status);

#ifdef HAVE_STRSIGNAL
    g_debug("child %i received genuine signal %d (%s)", child->pid, sig, strsignal(sig));
#else
    g_debug("child %i received genuine signal %d", child->pid, sig);
#endif /* HAVE_STRSIGNAL */

    if (G_UNLIKELY(!pink_trace_syscall(child->pid, sig))) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to resume child %i after genuine signal: %s", child->pid, g_strerror(errno));
            g_printerr("failed to resume child %i after genuine signal: %s\n", child->pid, g_strerror(errno));
            exit(-1);
        }
        return context_remove_child(ctx, child->pid);
    }
    g_debug("child %i was resumed after genuine signal", child->pid);
    return 0;
}

static int event_unknown(context_t *ctx, struct tchild *child, int status)
{
    int sig = WSTOPSIG(status);

#ifdef HAVE_STRSIGNAL
    g_info("unknown signal %#x (%s) received from child %i", sig, strsignal(sig), child->pid);
#else
    g_info("unknown signal %#x received from child %i", sig, child->pid);
#endif /* HAVE_STRSIGNAL */

    if (G_UNLIKELY(!pink_trace_syscall(child->pid, sig))) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to resume child %i after unknown signal %#x: %s",
                    child->pid, sig, g_strerror(errno));
            g_printerr("failed to resume child %i after unknown signal %#x: %s\n",
                    child->pid, sig, g_strerror(errno));
            exit(-1);
        }
        return context_remove_child(ctx, child->pid);
    }
    g_debug("child %i was resumed after unknown signal %#x", child->pid, sig);
    return 0;
}

int trace_loop(context_t *ctx)
{
    int status, exit_code;
    pid_t pid;
    pink_event_t event;
    struct tchild *child;

    exit_code = EXIT_SUCCESS;
    while (g_hash_table_size(ctx->children) > 0) {
        pid = waitpid(-1, &status, __WALL);
        if (G_UNLIKELY(0 > pid)) {
            if (EINTR == errno)
                continue;
            else if (ECHILD == errno)
                break;
            else {
                g_critical("waitpid failed: %s", g_strerror(errno));
                g_printerr("waitpid failed: %s\n", g_strerror(errno));
                exit(-1);
            }
        }
        child = tchild_find(ctx->children, pid);
        event = pink_event_decide(status);

        switch(event) {
            case PINK_EVENT_STOP:
                g_debug("child %i stopped", pid);
                if (NULL == child) {
                    /* Child is born before PTRACE_EVENT_FORK.
                     * Set her up but don't resume her until we receive the
                     * event.
                     */
                    g_debug("setting up prematurely born child %i", pid);
                    child = tchild_new(ctx->children, pid, false);
                    if (0 != event_setup(ctx, child))
                        return exit_code;
                }
                else {
                    g_debug("setting up child %i", child->pid);
                    if (0 != event_setup(ctx, child))
                        return exit_code;
                    if (0 != event_syscall(ctx, child))
                        return exit_code;
                }
                break;
            case PINK_EVENT_SYSCALL:
                if (0 != syscall_handle(ctx, child))
                    return exit_code;
                if (0 != event_syscall(ctx, child))
                    return exit_code;
                break;
            case PINK_EVENT_FORK:
            case PINK_EVENT_VFORK:
            case PINK_EVENT_CLONE:
                g_debug("child %i called %s", pid, pink_event_name(event));
                if (0 != event_fork(ctx, child))
                    return exit_code;
                if (0 != event_syscall(ctx, child))
                    return exit_code;
                break;
            case PINK_EVENT_EXEC:
                g_debug("child %i called execve()", pid);
                // Check for exec_lock
                if (G_UNLIKELY(LOCK_PENDING == child->sandbox->lock)) {
                    g_info("access to magic commands is now denied for child %i", child->pid);
                    child->sandbox->lock = LOCK_SET;
                }

                // Update child's bitness
                child->bitness = pink_bitness_get(child->pid);
                if (PINK_BITNESS_UNKNOWN == child->bitness) {
                    g_critical("failed to determine bitness of child %i: %s", child->pid, g_strerror(errno));
                    g_printerr("failed to determine bitness of child %i: %s\n", child->pid, g_strerror(errno));
                    exit(-1);
                }
                g_debug("updated child %i's bitness to %s mode", child->pid, pink_bitness_name(child->bitness));
                if (0 != event_syscall(ctx, child))
                    return exit_code;
                break;
            case PINK_EVENT_EXIT:
                if (0 != event_exit(ctx, pid, &exit_code))
                    return exit_code;
                break;
            case PINK_EVENT_GENUINE:
            case PINK_EVENT_TRAP:
                if (0 != event_genuine(ctx, child, status))
                    return exit_code;
                break;
            case PINK_EVENT_UNKNOWN:
                if (0 != event_unknown(ctx, child, status))
                    return exit_code;
                break;
            case PINK_EVENT_EXIT_GENUINE:
            case PINK_EVENT_EXIT_SIGNAL:
                if (NULL != child) {
                    g_warning("dead child %i is still being traced!", child->pid);
                    tchild_delete(ctx->children, child->pid);
                }
                break;
            default:
                g_assert_not_reached();
        }
    }
    return exit_code;
}

