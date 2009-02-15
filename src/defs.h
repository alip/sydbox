/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel
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

#ifndef SYDBOX_GUARD_DEFS_H
#define SYDBOX_GUARD_DEFS_H 1

#include <sysexits.h>
#include <sys/types.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif /* HAVE_SYS_REG_H */

/* environment */
#define ENV_WRITE       "SYDBOX_WRITE"
#define ENV_PREDICT     "SYDBOX_PREDICT"
#define ENV_NO_COLOUR   "SYDBOX_NO_COLOUR"

/* path.c */
struct pathnode {
    char *pathname;
    struct pathnode *next;
};

extern void pathnode_new(struct pathnode **head, const char *pathname);
extern void pathnode_free(struct pathnode **head);
extern int pathlist_init(struct pathnode **pathlist, const char *pathlist_env);
extern int pathlist_check(struct pathnode **pathlist, const char *pathname);

/* children.c */
/* Events */
enum {
    E_SETUP = 0,
    E_SETUP_PREMATURE,
    E_SYSCALL,
    E_FORK,
    E_EXECV,
    E_GENUINE,
    E_EXIT,
    E_EXIT_SIGNAL,
    E_UNKNOWN
};

/* Per process tracking data */
struct tchild {
    /* process id of the traced child */
    pid_t pid;
    /* we will get a stop signal from child and will setup tracing flags */
    int need_setup;
    /* child is in syscall */
    int in_syscall;
    /* original syscall number when a syscall is faked */
    unsigned long orig_syscall;
    /* faked syscall will fail with this error code */
    int error_code;
    struct tchild *next;
};

extern void tchild_new(struct tchild **head, pid_t pid);
extern void tchild_free(struct tchild **head);
extern void tchild_delete(struct tchild **head, pid_t pid);
extern struct tchild *tchild_find(struct tchild **head, pid_t pid);
extern void tchild_setup(struct tchild *child);
extern unsigned int tchild_event(struct tchild *child, int status);

/* context.c */
typedef struct {
    struct pathnode *write_prefixes;
    struct pathnode *predict_prefixes;
    struct tchild *children;
    /* first child pointer is kept for determining return code */
    struct tchild *eldest;
} context_t;

extern context_t *context_new(void);
extern void context_free(context_t *ctx);

/* util.c */
#define LOG_ERROR       1
#define LOG_WARNING     2
#define LOG_NORMAL      3
#define LOG_VERBOSE     4
#define LOG_DEBUG       5
int log_level;

#define NORMAL  "[00;00m"
#define MAGENTA "[00;35m"
#define PINK    "[01;35m"

extern void die(int err, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));
extern void lg(int level, const char *id, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 3, 4)));
extern void access_error(pid_t pid, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));
extern void *xmalloc(size_t size);
extern char *xstrndup(const char *s, size_t n);

/* realpath.c */
extern char *safe_realpath(const char *path, pid_t pid, int resolv, int *issymlink);

/* trace.c */
#define ADDR_MUL        ((64 == __WORDSIZE) ? 8 : 4)
#if defined(I386)
#define ORIG_ACCUM      (4 * ORIG_EAX)
#define ACCUM           (4 * EAX)
#define PARAM1          (4 * EBX)
#define PARAM2          (4 * ECX)
#define PARAM3          (4 * EDX)
#elif defined(X86_64)
#define ORIG_ACCUM      (8 * ORIG_RAX)
#define ACCUM           (8 * RAX)
#define PARAM1          (8 * RDI)
#define PARAM2          (8 * RSI)
#define PARAM3          (8 * RDX)
#endif

extern int ptrace_get_syscall(pid_t pid);
extern void ptrace_set_syscall(pid_t pid, int syscall);
extern void ptrace_get_string(pid_t pid, int param, char *dest, size_t len);
extern void ptrace_set_string(pid_t pid, int param, char *src, size_t len);

/* syscall.c */
extern int syscall_handle(context_t *ctx, struct tchild *child);

#endif /* SYDBOX_GUARD_DEFS_H */