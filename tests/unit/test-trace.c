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

#include <asm/unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <glib.h>

#include "../src/trace.h"

#include "test-helpers.h"

#if defined(I386) || defined(IA64) || defined(POWERPC)
#define CHECK_PERSONALITY 0
#elif defined(X86_64)
#define CHECK_PERSONALITY 1
#else
#error unsupported architecture
#endif

static void cleanup(void)
{
    unlink("arnold_layne");
}

static void test1(void)
{
    int ret, status;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
    }
    else { // parent
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");

        ret = trace_event(status);
        XFAIL_UNLESS(E_STOP == ret, "Expected E_STOP got %d\n", ret);

        trace_kill(pid);
    }
}

static void test2(void)
{
    int ret, status;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        sleep(1);
    }
    else { // parent
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        XFAIL_IF(0 > trace_syscall(pid, 0), "failed to resume child: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");

        ret = trace_event(status);
        XFAIL_UNLESS(E_SYSCALL == ret, "Expected E_SYSCALL got %d\n", ret);

        trace_kill(pid);
    }
}

static void test3(void)
{
    int ret, status;
    pid_t pid, cpid;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);

        cpid = fork();
        if (0 > cpid) {
            g_printerr("fork() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
        pause();
    }
    else {
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child, it will stop at the fork() */
        XFAIL_IF(0 > trace_cont(pid), "trace_cont() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");

        /* Check the event */
        ret = trace_event(status);
        XFAIL_UNLESS(E_FORK == ret, "Expected E_FORK got %d\n", ret);

        trace_kill(pid);
    }
}

static void test4(void)
{
    int ret, status;
    pid_t pid, cpid;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);

        cpid = vfork();
        if (0 > cpid) {
            g_printerr("vfork() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
        pause();
    }
    else {
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child, it will stop at the vfork() */
        XFAIL_IF(0 > trace_cont(pid), "trace_cont() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");

        /* Check the event */
        ret = trace_event(status);
        XFAIL_UNLESS(E_VFORK == ret, "Expected E_VFORK got %d\n", ret);

        trace_kill(pid);
    }
}

static void test5(void)
{
    int ret, status;
    pid_t pid;
    char *myargv[] = { "/bin/true", NULL };
    char *myenviron[] = { NULL };

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);

        execve(myargv[0], myargv, myenviron);
        g_printerr("execve() failed: %s\n", g_strerror(errno));
        _exit(EXIT_FAILURE);
    }
    else {
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child, it will stop at the execve() */
        XFAIL_IF(0 > trace_cont(pid), "trace_cont() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");

        /* Check the event */
        ret = trace_event(status);
        XFAIL_UNLESS(E_EXEC == ret, "Expected E_EXEC got %d\n", ret);

        trace_kill(pid);
    }
}

static void test6(void)
{
    int ret, status;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        kill(getpid(), SIGINT);
    }
    else { // parent
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child, it will receive a SIGINT */
        XFAIL_IF(0 > trace_cont(pid), "trace_cont() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);

        /* Check the event */
        ret = trace_event(status);
        XFAIL_UNLESS(E_GENUINE == ret, "Expected E_GENUINE got %d\n", ret);

        trace_kill(pid);
    }
}

static void test7(void)
{
    int ret, status;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        exit(EXIT_SUCCESS);
    }
    else { // parent
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child, it will exit normally */
        XFAIL_IF(0 > trace_cont(pid), "trace_cont() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);

        /* Check the event */
        ret = trace_event(status);
        XFAIL_UNLESS(E_EXIT == ret, "Expected E_EXIT got %d\n", ret);

        trace_kill(pid);
    }
}

static void test8(void)
{
    int ret, status;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        pause();
    }
    else { // parent
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child and kill it with a signal */
        XFAIL_IF(0 > trace_cont(pid), "trace_cont() failed: %s\n", g_strerror(errno));
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);

        /* Check the event */
        ret = trace_event(status);
        XFAIL_UNLESS(E_EXIT_SIGNAL == ret, "Expected E_EXIT_SIGNAL got %d\n", ret);

        trace_kill(pid);
    }
}

static void test9(void)
{
    int ret, status;
    long sno;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDONLY);
        pause();
    }
    else { // parent
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child and it will stop at the next system call */
        XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");

        /* Check the system call number */
        XFAIL_IF(0 > trace_get_syscall(pid, &sno), "failed to get system call: %s\n", g_strerror(errno));
        XFAIL_UNLESS(__NR_open == sno, "expected __NR_open, got %d\n", sno);

        trace_kill(pid);
    }
}

static void test10(void)
{
    int ret, status;
    long sno;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDONLY);
        pause();
    }
    else { // parent
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child and it will stop at the next system call */
        XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");

        XFAIL_IF(0 > trace_set_syscall(pid, 0xbadca11), "failed to set system call: %s", g_strerror(errno));

        /* Resume the child and it will stop at the end of the system call */
        XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");

        /* Check the system call number */
        XFAIL_IF(0 > trace_get_syscall(pid, &sno), "failed to get system call: %s\n", g_strerror(errno));
        XFAIL_UNLESS(0xbadca11 == sno, "expected 0xbadca11, got %d\n", sno);

        trace_kill(pid);
    }
}

static void test11(void)
{
    int ret, status;
    char *path;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDONLY);
        pause();
    }
    else { // parent
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child and it will stop at the next system call */
        XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");

        /* Check the path argument */
        path = trace_get_path(pid, CHECK_PERSONALITY, 0);
        XFAIL_IF(NULL == path, "failed to get string: %s\n", g_strerror(errno));
        XFAIL_UNLESS(0 == strncmp(path, "/dev/null", 10), "expected `/dev/null' got `%s'", path);

        free(path);
        trace_kill(pid);
    }
}

static void test12(void)
{
    int ret, status;
    char *path;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        openat(-1, "/dev/null", O_RDONLY);
        pause();
    }
    else { // parent
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child and it will stop at the next system call */
        XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");

        /* Check the path argument */
        path = trace_get_path(pid, CHECK_PERSONALITY, 1);
        XFAIL_IF(NULL == path, "failed to get string: %s\n", g_strerror(errno));
        XFAIL_UNLESS(0 == strncmp(path, "/dev/null", 10), "expected `/dev/null' got `%s'", path);

        free(path);
        trace_kill(pid);
    }
}

static void test13(void)
{
    int ret, status;
    char *path;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        symlinkat("emily", AT_FDCWD, "arnold_layne");
        pause();
    }
    else { // parent
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child and it will stop at the next system call */
        XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");

        /* Check the path argument */
        path = trace_get_path(pid, CHECK_PERSONALITY, 2);
        XFAIL_IF(NULL == path, "failed to get string: %s\n", g_strerror(errno));
        XFAIL_UNLESS(0 == strncmp(path, "arnold_layne", 13), "expected `arnold_layne' got `%s'", path);

        free(path);
        trace_kill(pid);
    }
}

static void test14(void)
{
    int ret, status;
    char *path;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me())
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
        kill(getpid(), SIGSTOP);
        linkat(AT_FDCWD, "emily", AT_FDCWD, "arnold_layne", 0600);
        pause();
    }
    else { // parent
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child and it will stop at the next system call */
        XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");

        /* Check the path argument */
        path = trace_get_path(pid, CHECK_PERSONALITY, 3);
        XFAIL_IF(NULL == path, "failed to get string: %s\n", g_strerror(errno));
        XFAIL_UNLESS(0 == strncmp(path, "arnold_layne", 13), "expected `arnold_layne' got `%s'", path);

        free(path);
        trace_kill(pid);
    }
}

int main(int argc, char **argv)
{
    atexit(cleanup);

    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/trace/event/stop", test1);
    g_test_add_func("/trace/event/syscall", test2);
    g_test_add_func("/trace/event/fork", test3);
    g_test_add_func("/trace/event/vfork", test4);
    g_test_add_func("/trace/event/exec", test5);
    g_test_add_func("/trace/event/genuine", test6);
    g_test_add_func("/trace/event/exit/normal", test7);
    g_test_add_func("/trace/event/exit/signal", test8);

    g_test_add_func("/trace/syscall/get", test9);
    g_test_add_func("/trace/syscall/set", test10);

    g_test_add_func("/trace/path/get/first", test11);
    g_test_add_func("/trace/path/get/second", test12);
    g_test_add_func("/trace/path/get/third", test13);
    g_test_add_func("/trace/path/get/fourth", test14);

    return g_test_run();
}

