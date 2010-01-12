/* vim: set et ts=4 sts=4 sw=4 fdm=syntax : */

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <glib.h>

#include "syd-config.h"
#include "syd-trace.h"

#include "test-helpers.h"

#if defined(I386) || defined(IA64) || defined(POWERPC)
#define CHECK_PERSONALITY 0
#elif defined(X86_64)
#define CHECK_PERSONALITY 1
#else
#error unsupported architecture
#endif

#if defined(I386) || defined(POWERPC)
#define DECODE_SOCKETCALL (true)
#elif defined(IA64) || defined(X86_64)
#define DECODE_SOCKETCALL (false)
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
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

static void test15(void)
{
    int status;
    pid_t pid;
    struct stat buf;

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
        kill(getpid(), SIGSTOP);
        if (stat("/dev/sydbox", &buf) < 0) {
            g_printerr("stat() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
        if (buf.st_rdev == 259) /* /dev/null */
            _exit(EXIT_SUCCESS);
        g_printerr("buf.st_rdev=%d\n", buf.st_rdev);
        _exit(EXIT_FAILURE);
    }
    else { // parent
        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child and it will stop at the next system call */
        XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");

        /* Fake the stat argument and deny the system call */
        XFAIL_IF(0 > trace_fake_stat(pid, CHECK_PERSONALITY), "trace_fake_stat() failed: %s\n", g_strerror(errno));
        XFAIL_IF(0 > trace_set_syscall(pid, 0xbadca11), "failed to deny stat: %s\n", g_strerror(errno));

        /* Resume the child, it will stop at the end of stat() call */
        XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");
        XFAIL_IF(0 > trace_set_return(pid, 0), "trace_set_return() failed: %s\n", g_strerror(errno));

        /* Resume the child, it will exit */
        XFAIL_IF(0 > trace_cont(pid), "trace_cont() failed: %s\n", g_strerror(errno));
        waitpid(pid, &status, 0);
        XFAIL_UNLESS(WIFEXITED(status), "child didn't exit");
        XFAIL_UNLESS(WEXITSTATUS(status) == EXIT_SUCCESS, "failed to fake stat argument");
    }
}

static void test16(void)
{
    int status;
    long fd;
    int pfd[2];
    char strfd[16];
    pid_t pid;

    if (0 > pipe(pfd))
        XFAIL("pipe() failed: %s\n", g_strerror(errno));

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        int len;
        struct sockaddr_un addr;

        close(pfd[0]);
        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            g_printerr("socket() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
        snprintf(strfd, 16, "%i", fd);
        write(pfd[1], strfd, 16);
        close(pfd[1]);

        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, "/dev/null");
        len = strlen(addr.sun_path) + sizeof(addr.sun_family);

        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }

        kill(getpid(), SIGSTOP);
        connect(fd, (struct sockaddr *)&addr, len);
        close(fd);
        pause();
    }
    else { // parent
        int realfd;

        close(pfd[1]);
        read(pfd[0], strfd, 16);
        realfd = atoi(strfd);
        close(pfd[0]);

        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child, until the connect() call. */
        for (unsigned int i = 0; i < 2; i++) {
            XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
            waitpid(pid, &status, 0);
            XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");
        }

        /* Check the fd argument */
        XFAIL_IF(!trace_get_fd(pid, CHECK_PERSONALITY, DECODE_SOCKETCALL, &fd),
                "failed to get file descriptor: %s\n", g_strerror(errno));
        XFAIL_UNLESS(fd == realfd, "wrong file descriptor got:%d expected:%d\n", fd, realfd);

        trace_kill(pid);
    }
}

static void test17(void)
{
    int status, pfd[2];
    pid_t pid;
    char strfd[16];

    if (0 > pipe(pfd))
        XFAIL("pipe() failed: %s\n", g_strerror(errno));

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        int fd, len;
        struct sockaddr_un addr;

        close(pfd[0]);
        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            g_printerr("socket() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
        snprintf(strfd, 16, "%i", fd);
        write(pfd[1], strfd, 16);
        close(pfd[1]);

        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, "/dev/null");
        len = strlen(addr.sun_path) + sizeof(addr.sun_family);

        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }

        kill(getpid(), SIGSTOP);
        connect(fd, (struct sockaddr *)&addr, len);
        close(fd);
        pause();
    }
    else { // parent
        long fd, realfd;
        struct sydbox_addr *addr;

        close(pfd[1]);
        read(pfd[0], strfd, 16);
        realfd = atoi(strfd);
        close(pfd[0]);

        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child, until the connect() call. */
        for (unsigned int i = 0; i < 2; i++) {
            XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
            waitpid(pid, &status, 0);
            XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");
        }

        /* Check the address. */
        addr = trace_get_addr(pid, CHECK_PERSONALITY, 1, DECODE_SOCKETCALL, &fd);
        XFAIL_IF(NULL == addr, "trace_get_addr() failed: %s\n", g_strerror(errno));
        XFAIL_UNLESS(0 == strncmp(addr->u.sun_path, "/dev/null", 10),
                "wrong address got:`%s' expected:`/dev/null'", addr->u.sun_path);
        XFAIL_UNLESS(addr->abstract == false, "non-abstract socket marked abstract");
        XFAIL_UNLESS(fd == realfd, "wrong file descriptor got:%d expected:%d\n", fd, realfd);
        XFAIL_UNLESS(addr->family == AF_UNIX, "wrong family got:%d expected:%d\n", addr->family, AF_UNIX);
        XFAIL_UNLESS(addr->port[0] == -1, "wrong port got:%d expected:-1\n", addr->port[0]);

        g_free(addr);
        trace_kill(pid);
    }
}

static void test18(void)
{
    int status, pfd[2];
    pid_t pid;
    char strfd[16];

    if (0 > pipe(pfd))
        XFAIL("pipe() failed: %s\n", g_strerror(errno));

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        int fd, len;
        struct sockaddr_un addr;

        close(pfd[0]);
        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            g_printerr("socket() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
        snprintf(strfd, 16, "%i", fd);
        write(pfd[1], strfd, 16);
        close(pfd[1]);

        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, "X/dev/null");
        len = strlen(addr.sun_path) + sizeof(addr.sun_family);
        addr.sun_path[0] = '\0';

        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }

        kill(getpid(), SIGSTOP);
        connect(fd, (struct sockaddr *)&addr, len);
        close(fd);
        pause();
    }
    else { // parent
        long fd, realfd;
        struct sydbox_addr *addr;

        close(pfd[1]);
        read(pfd[0], strfd, 16);
        realfd = atoi(strfd);
        close(pfd[0]);

        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child, until the connect() call. */
        for (unsigned int i = 0; i < 2; i++) {
            XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
            waitpid(pid, &status, 0);
            XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");
        }

        /* Check the address. */
        addr = trace_get_addr(pid, CHECK_PERSONALITY, 1, DECODE_SOCKETCALL, &fd);
        XFAIL_IF(NULL == addr, "trace_get_addr() failed: %s\n", g_strerror(errno));
        XFAIL_UNLESS(0 == strncmp(addr->u.sun_path, "/dev/null", 10),
                "wrong address got:`%s' expected:`/dev/null'", addr->u.sun_path);
        XFAIL_UNLESS(addr->abstract == true, "abstract socket marked non-abstract");
        XFAIL_UNLESS(fd == realfd, "wrong file descriptor got:%d expected:%d\n", fd, realfd);
        XFAIL_UNLESS(addr->family == AF_UNIX, "wrong family got:%d expected:%d\n", addr->family, AF_UNIX);
        XFAIL_UNLESS(addr->port[0] == -1, "wrong port got:%d expected:-1\n", addr->port[0]);

        g_free(addr);
        trace_kill(pid);
    }
}

static void test19(void)
{
    int status, pfd[2];
    pid_t pid;
    char strfd[16];

    if (0 > pipe(pfd))
        XFAIL("pipe() failed: %s\n", g_strerror(errno));

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        int fd;
        struct sockaddr_in addr;

        close(pfd[0]);
        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            g_printerr("socket() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
        snprintf(strfd, 16, "%i", fd);
        write(pfd[1], strfd, 16);
        close(pfd[1]);

        addr.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &(addr.sin_addr));
        addr.sin_port = htons(23456);

        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }

        kill(getpid(), SIGSTOP);
        connect(fd, (struct sockaddr *)&addr, sizeof(addr));
        close(fd);
        pause();
    }
    else { // parent
        long fd, realfd;
        struct sydbox_addr *addr;
        char ip[100] = { 0 };

        close(pfd[1]);
        read(pfd[0], strfd, 16);
        realfd = atoi(strfd);
        close(pfd[0]);

        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child, until the connect() call. */
        for (unsigned int i = 0; i < 2; i++) {
            XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
            waitpid(pid, &status, 0);
            XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");
        }

        /* Check the address. */
        addr = trace_get_addr(pid, CHECK_PERSONALITY, 1, DECODE_SOCKETCALL, &fd);
        XFAIL_IF(NULL == addr, "trace_get_addr() failed: %s\n", g_strerror(errno));
        XFAIL_IF(NULL == inet_ntop(AF_INET, &addr->u.sin_addr, ip, sizeof(ip)), "inet_ntop failed: %s\n",
                    g_strerror(errno));
        XFAIL_UNLESS(0 == strncmp(ip, "127.0.0.1", 10),
                "wrong address got:`%s' expected:`127.0.0.1'", ip);
        XFAIL_UNLESS(fd == realfd, "wrong file descriptor got:%d expected:%d\n", fd, realfd);
        XFAIL_UNLESS(addr->family == AF_INET, "wrong family got:%d expected:%d\n", addr->family, AF_INET);
        XFAIL_UNLESS(addr->port[0] == 23456, "wrong port got:%d expected:23456\n", addr->port[0]);

        g_free(addr);
        trace_kill(pid);
    }
}

#if HAVE_IPV6
static void test20(void)
{
    int status, pfd[2];
    pid_t pid;
    char strfd[16];

    if (0 > pipe(pfd))
        XFAIL("pipe() failed: %s\n", g_strerror(errno));

    pid = fork();
    if (0 > pid)
        XFAIL("fork() failed: %s\n", g_strerror(errno));
    else if (0 == pid) { // child
        int fd;
        struct sockaddr_in6 addr;

        close(pfd[0]);
        if ((fd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
            g_printerr("socket() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }
        snprintf(strfd, 16, "%i", fd);
        write(pfd[1], strfd, 16);
        close(pfd[1]);

        addr.sin6_family = AF_INET6;
        inet_pton(AF_INET6, "::1", &(addr.sin6_addr));
        addr.sin6_port = htons(23456);

        if (0 > trace_me()) {
            g_printerr("trace_me() failed: %s\n", g_strerror(errno));
            _exit(EXIT_FAILURE);
        }

        kill(getpid(), SIGSTOP);
        connect(fd, (struct sockaddr *)&addr, sizeof(addr));
        close(fd);
        pause();
    }
    else { // parent
        long fd, realfd;
        struct sydbox_addr *addr;
        char ip[100] = { 0 };

        close(pfd[1]);
        read(pfd[0], strfd, 16);
        realfd = atoi(strfd);
        close(pfd[0]);

        waitpid(pid, &status, 0);

        XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGSTOP\n");
        XFAIL_IF(0 > trace_setup(pid), "failed to set tracing options: %s\n", g_strerror(errno));

        /* Resume the child, until the connect() call. */
        for (unsigned int i = 0; i < 2; i++) {
            XFAIL_IF(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s\n", g_strerror(errno));
            waitpid(pid, &status, 0);
            XFAIL_UNLESS(WIFSTOPPED(status), "child didn't stop by sending itself SIGTRAP\n");
        }

        /* Check the address. */
        addr = trace_get_addr(pid, CHECK_PERSONALITY, 1, DECODE_SOCKETCALL, &fd);
        XFAIL_IF(NULL == ip, "trace_get_addr() failed: %s\n", g_strerror(errno));
        XFAIL_IF(NULL == inet_ntop(AF_INET6, &addr->u.sin6_addr, ip, sizeof(ip)), "inet_ntop failed: %s\n",
                    g_strerror(errno));
        XFAIL_UNLESS(0 == strncmp(ip, "::1", 4),
                "wrong address got:`%s' expected:`::1'", ip);
        XFAIL_UNLESS(fd == realfd, "wrong file descriptor got:%d expected:%d\n", fd, realfd);
        XFAIL_UNLESS(addr->family == AF_INET6, "wrong family got:%d expected:%d\n", addr->family, AF_INET6);
        XFAIL_UNLESS(addr->port[0] == 23456, "wrong port got:%d expected:23456\n", addr->port[0]);

        g_free(addr);
        trace_kill(pid);
    }
}
#endif /* HAVE_IPV6 */

static void no_log(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
}

int main(int argc, char **argv)
{
    atexit(cleanup);

    g_setenv(ENV_NO_CONFIG, "1", 1);
    sydbox_config_load(NULL, NULL);

    g_test_init(&argc, &argv, NULL);

    g_log_set_default_handler(no_log, NULL);

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

    g_test_add_func("/trace/stat/fake", test15);

    g_test_add_func("/trace/socket/fd", test16);
    g_test_add_func("/trace/socket/addr/unix", test17);
    g_test_add_func("/trace/socket/addr/unix-abstract", test18);
    g_test_add_func("/trace/socket/addr/inet", test19);
#if HAVE_IPV6
    g_test_add_func("/trace/socket/addr/inet6", test20);
#endif /* HAVE_IPV6 */

    return g_test_run();
}

