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
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <glib.h>

#include "syd-children.h"
#include "syd-config.h"
#include "syd-dispatch.h"
#include "syd-log.h"
#include "syd-loop.h"
#include "syd-proc.h"
#include "syd-syscall.h"
#include "syd-trace.h"

// Event handlers
static int xsetup(context_t *ctx, struct tchild *child)
{
    if (0 > trace_setup(child->pid)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to set tracing options: %s", g_strerror(errno));
            g_printerr("failed to set tracing options: %s\n", g_strerror (errno));
            exit(-1);
        }
        return context_remove_child(ctx, child->pid);
    }
    else
        child->flags &= ~TCHILD_NEEDSETUP;
    return 0;
}

static int xsyscall(context_t *ctx, struct tchild *child)
{
    if (0 > trace_syscall(child->pid, 0)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to resume child %i: %s", child->pid, g_strerror (errno));
            g_printerr("failed to resume child %i: %s\n", child->pid, g_strerror (errno));
            exit(-1);
        }
        return context_remove_child(ctx, child->pid);
    }
    return 0;
}

static int xfork(context_t *ctx, struct tchild *child)
{
    unsigned long childpid;
    struct tchild *newchild;

    // Get new child's pid
    if (G_UNLIKELY(0 > trace_geteventmsg(child->pid, &childpid))) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to get the pid of the newborn child: %s", g_strerror(errno));
            g_printerr("failed to get the pid of the newborn child: %s\n", g_strerror (errno));
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
        xsyscall(ctx, newchild);
    }
    return 0;
}

static int xgenuine(context_t * ctx, struct tchild *child, int status)
{
    int sig = WSTOPSIG(status);

#ifdef HAVE_STRSIGNAL
    g_debug("child %i received genuine signal %d (%s)", child->pid, sig, strsignal(sig));
#else
    g_debug("child %i received genuine signal %d", child->pid, sig);
#endif /* HAVE_STRSIGNAL */

    if (G_UNLIKELY(0 > trace_syscall(child->pid, sig))) {
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

static int xunknown(context_t *ctx, struct tchild *child, int status)
{
    int sig = WSTOPSIG(status);

#ifdef HAVE_STRSIGNAL
    g_info("unknown signal %#x (%s) received from child %i", sig, strsignal(sig), child->pid);
#else
    g_info("unknown signal %#x received from child %i", sig, child->pid);
#endif /* HAVE_STRSIGNAL */

    if (G_UNLIKELY(0 > trace_syscall(child->pid, sig))) {
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
    int status, ret, exit_code;
    unsigned int event;
    pid_t pid;
    struct tchild *child;

    exit_code = ret = EXIT_SUCCESS;
    while (NULL != ctx->children) {
        pid = waitpid(-1, &status, __WALL);
        if (G_UNLIKELY(0 > pid)) {
            if (ECHILD == errno)
                break;
            else {
                g_critical("waitpid failed: %s", g_strerror(errno));
                g_printerr("waitpid failed: %s\n", g_strerror(errno));
                exit(-1);
            }
        }
        child = tchild_find(ctx->children, pid);
        event = trace_event(status);
        g_assert(NULL != child || E_STOP == event || E_EXIT == event || E_EXIT_SIGNAL == event);

        switch(event) {
            case E_STOP:
                g_debug("child %i stopped", pid);
                if (NULL == child) {
                    /* Child is born before PTRACE_EVENT_FORK.
                     * Set her up but don't resume her until we receive the
                     * event.
                     */
                    g_debug("setting up prematurely born child %i", pid);
                    child = tchild_new(ctx->children, pid, false);
                    ret = xsetup(ctx, child);
                    if (0 != ret)
                        return exit_code;
                }
                else {
                    g_debug("setting up child %i", child->pid);
                    ret = xsetup(ctx, child);
                    if (0 != ret)
                        return exit_code;
                    ret = xsyscall(ctx, child);
                    if (0 != ret)
                        return exit_code;
                }
                break;
            case E_SYSCALL:
                ret = syscall_handle(ctx, child);
                if (0 != ret)
                    return exit_code;
                ret = xsyscall(ctx, child);
                if (0 != ret)
                    return exit_code;
                break;
            case E_FORK:
            case E_VFORK:
            case E_CLONE:
                g_debug("child %i called %s()", pid,
                        (event == E_FORK)
                            ? "fork"
                            : (event == E_VFORK) ? "vfork" : "clone");
                ret = xfork(ctx, child);
                if (0 != ret)
                    return exit_code;
                ret = xsyscall(ctx, child);
                if (0 != ret)
                    return exit_code;
                break;
            case E_EXEC:
                g_debug("child %i called execve()", pid);
                // Check for exec_lock
                if (G_UNLIKELY(LOCK_PENDING == child->sandbox->lock)) {
                    g_info("access to magic commands is now denied for child %i", child->pid);
                    child->sandbox->lock = LOCK_SET;
                }

                // Update child's personality
                child->personality = trace_personality(child->pid);
                if (0 > child->personality) {
                    g_critical("failed to determine personality of child %i: %s", child->pid, g_strerror(errno));
                    g_printerr("failed to determine personality of child %i: %s\n", child->pid, g_strerror(errno));
                    exit(-1);
                }
                g_debug("updated child %i's personality to %s mode", child->pid, dispatch_mode(child->personality));
                ret = xsyscall(ctx, child);
                if (0 != ret)
                    return exit_code;
                break;
            case E_GENUINE:
                ret = xgenuine(ctx, child, status);
                if (0 != ret)
                    return exit_code;
                break;
            case E_EXIT:
                if (G_UNLIKELY(ctx->eldest == pid)) {
                    // Eldest child, keep return value.
                    exit_code = WEXITSTATUS(status);
                    if (EXIT_SUCCESS != exit_code)
                        g_message("eldest child %i exited with return code %d", pid, exit_code);
                    else
                        g_info("eldest child %i exited with return code %d", pid, exit_code);
                    if (!sydbox_config_get_wait_all()) {
                        g_hash_table_foreach(ctx->children, tchild_cont_one, NULL);
                        g_hash_table_destroy(ctx->children);
                        ctx->children = NULL;
                        return exit_code;
                    }
                }
                else
                    g_debug("child %i exited with return code %d", pid, ret);
                tchild_delete(ctx->children, pid);
                break;
            case E_EXIT_SIGNAL:
                if (G_UNLIKELY(ctx->eldest == pid)) {
                    exit_code = 128 + WTERMSIG(status);
#ifdef HAVE_STRSIGNAL
                    g_message("eldest child %i exited with signal %d (%s)", pid,
                            WTERMSIG(status), strsignal(WTERMSIG(status)));
#else
                    g_message("eldest child %i exited with signal %d", pid, WTERMSIG(status));
#endif /* HAVE_STRSIGNAL */
                    if (!sydbox_config_get_wait_all()) {
                        g_hash_table_foreach(ctx->children, tchild_cont_one, NULL);
                        g_hash_table_destroy(ctx->children);
                        ctx->children = NULL;
                        return exit_code;
                    }
                }
                else {
#ifdef HAVE_STRSIGNAL
                    g_info("child %i exited with signal %d (%s)", pid,
                            WTERMSIG(status), strsignal(WTERMSIG(status)));
#else
                    g_info("child %i exited with signal %d", pid, WTERMSIG(status));
#endif /* HAVE_STRSIGNAL */
                }

                tchild_delete(ctx->children, pid);
                break;
            case E_UNKNOWN:
                ret = xunknown(ctx, child, status);
                if (0 != ret)
                    return exit_code;
                break;
            default:
                g_assert_not_reached();
        }
    }
    return exit_code;
}

