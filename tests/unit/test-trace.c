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

#ifndef _ATFILE_SOURCE
#define _ATFILE_SOURCE 1
#endif // !_ATFILE_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <glib.h>

#include "../src/trace.h"

static void test1(void)
{
    int ret, status;
    pid_t pid;

    pid = fork();
    if (0 > pid) {
        g_printerr("fork() failed: %s\n", g_strerror(errno));
        g_assert(FALSE);
    }
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
    }
    else { // parent
        waitpid(pid, &status, 0);

        if (!WIFSTOPPED(status)) {
            g_printerr("child %i didn't stop by sending itself SIGSTOP\n", pid);
            g_assert(FALSE);
        }

        ret = trace_event(status);
        if (E_STOP != ret) {
            g_printerr("Expected E_STOP got %d\n", ret);
            g_assert(FALSE);
        }

        trace_kill(pid);
    }
}

static void test2(void)
{
    int ret, status;
    pid_t pid;

    pid = fork();
    if (0 > pid) {
        g_printerr("fork() failed: %s\n", g_strerror(errno));
        g_assert(FALSE);
    }
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        sleep(1);
    }
    else { // parent
        waitpid(pid, &status, 0);

        if (!WIFSTOPPED(status)) {
            g_printerr("child %i didn't stop by sending itself SIGSTOP\n", pid);
            g_assert(FALSE);
        }
        if (0 > trace_setup(pid)) {
            g_printerr("Failed to set tracing options: %s", g_strerror(errno));
            g_assert(FALSE);
        }

        /* Resume the child, it will stop at the next system call. */
        if (0 > trace_syscall(pid, 0)) {
            g_printerr("Failed to resume child: %s", g_strerror(errno));
            g_assert(FALSE);
        }

        waitpid(pid, &status, 0);

        if (!WIFSTOPPED(status)) {
            g_printerr("child %i didn't stop by sending itself SIGTRAP\n", pid);
            g_assert(FALSE);
        }

        ret = trace_event(status);
        if (E_SYSCALL != ret) {
            g_printerr("Expected E_SYSCALL got %d", ret);
            g_assert(FALSE);
        }

        trace_kill(pid);
    }
}

static void test7(void)
{
    int ret, status;
    pid_t pid;

    pid = fork();
    if (0 > pid) {
        g_printerr("fork() failed: %s\n", g_strerror(errno));
        g_assert(FALSE);
    }
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        kill(getpid(), SIGINT);
    }
    else { // parent
        waitpid(pid, &status, 0);

        if (!WIFSTOPPED(status)) {
            g_printerr("child %i didn't stop by sending itself SIGSTOP\n", pid);
            g_assert(FALSE);
        }
        if (0 > trace_setup(pid)) {
            g_printerr("failed to set tracing options: %s", g_strerror(errno));
            g_assert(FALSE);
        }

        /* Resume the child, it will receive a SIGINT */
        if (0 > trace_cont(pid)) {
            g_printerr("trace_cont() failed: %s", g_strerror(errno));
            g_assert(FALSE);
        }
        waitpid(pid, &status, 0);

        /* Check the event */
        ret = trace_event(status);
        if (E_GENUINE != ret) {
            g_printerr("Expected E_GENUINE got %d", ret);
            g_assert(FALSE);
        }

        trace_kill(pid);
    }
}

static void test8(void)
{
    int ret, status;
    pid_t pid;

    pid = fork();
    if (0 > pid) {
        g_printerr("fork() failed: %s\n", g_strerror(errno));
        g_assert(FALSE);
    }
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        exit(EXIT_SUCCESS);
    }
    else { // parent
        waitpid(pid, &status, 0);

        if (!WIFSTOPPED(status)) {
            g_printerr("child %i didn't stop by sending itself SIGSTOP\n", pid);
            g_assert(FALSE);
        }
        if (0 > trace_setup(pid)) {
            g_printerr("failed to set tracing options: %s", g_strerror(errno));
            g_assert(FALSE);
        }

        /* Resume the child, it will exit normally */
        if (0 > trace_cont(pid)) {
            g_printerr("trace_cont() failed: %s", g_strerror(errno));
            g_assert(FALSE);
        }
        waitpid(pid, &status, 0);

        /* Check the event */
        ret = trace_event(status);
        if (E_EXIT != ret) {
            g_printerr("Expected E_EXIT got %d", ret);
            g_assert(FALSE);
        }

        trace_kill(pid);
    }
}

static void test9(void)
{
    int ret, status;
    pid_t pid;

    pid = fork();
    if (0 > pid) {
        g_printerr("fork() failed: %s\n", g_strerror(errno));
        g_assert(FALSE);
    }
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        pause();
    }
    else { // parent
        waitpid(pid, &status, 0);

        if (!WIFSTOPPED(status)) {
            g_printerr("child %i didn't stop by sending itself SIGSTOP\n", pid);
            g_assert(FALSE);
        }
        if (0 > trace_setup(pid)) {
            g_printerr("failed to set tracing options: %s", g_strerror(errno));
            g_assert(FALSE);
        }

        /* Resume the child and kill it with a signal */
        if (0 > trace_cont(pid)) {
            g_printerr("trace_cont() failed: %s", g_strerror(errno));
            g_assert(FALSE);
        }
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);

        /* Check the event */
        ret = trace_event(status);
        if (E_EXIT_SIGNAL != ret) {
            g_printerr("Expected E_EXIT_SIGNAL got %d", ret);
            g_assert(FALSE);
        }
    }
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/trace/event/stop", test1);
    g_test_add_func("/trace/event/syscall", test2);
/* TODO
 *  g_test_add_func("/trace/event/fork", test3);
 *  g_test_add_func("/trace/event/vfork", test4);
 *  g_test_add_func("/trace/event/clone", test5);
 *  g_test_add_func("/trace/event/execv", test6);
 */
    g_test_add_func("/trace/event/genuine", test7);
    g_test_add_func("/trace/event/exit/normal", test8);
    g_test_add_func("/trace/event/exit/signal", test9);

    return g_test_run();
}

