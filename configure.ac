dnl vim: set sw=4 sts=4 ts=4 noet ft=config foldmethod=marker foldmarker={{{,}}} :

dnl {{{ program, version
AC_PREREQ(2.59)
AC_INIT([src/syd-main.c])
AC_CANONICAL_HOST

VERSION_MAJOR=0
VERSION_MINOR=7
VERSION_MICRO=0
VERSION_SUFFIX=
VERSION_FULL="$VERSION_MAJOR.$VERSION_MINOR.$VERSION_MICRO$VERSION_SUFFIX"
VERSION="$VERSION_FULL"

AC_SUBST([VERSION_MAJOR])
AC_SUBST([VERSION_MINOR])
AC_SUBST([VERSION_FULL])

AM_INIT_AUTOMAKE(sydbox, [$VERSION_FULL])

dnl {{{ git revision
AC_MSG_CHECKING([for git head])
if test -d "${GIT_DIR:-${ac_top_srcdir:-./}/.git}" ; then
	GIT_HEAD=`git describe 2>/dev/null`
	if test -z ${GIT_HEAD} ; then
		GIT_HEAD=`git rev-parse --short HEAD`
	fi
	if test -n "`git diff-index -m --name-only HEAD`" ; then
		GIT_HEAD=${GIT_HEAD}-dirty
	fi
	if test -n "${GIT_HEAD}" ; then
		GIT_HEAD="-${GIT_HEAD}"
	fi
fi
AC_MSG_RESULT([$GIT_HEAD])
AC_SUBST([GIT_HEAD])
dnl }}}
dnl }}}

dnl {{{ toolchain checks
AC_PROG_CC
AC_PROG_CC_C99
if test x"$ac_cv_prog_cc_c99" = x"no"; then
	AC_MSG_ERROR([sydbox requires a C compiler that supports ISO C99!])
fi
AC_GNU_SOURCE
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_SED
dnl }}}

dnl {{{ Check headers
AC_CHECK_HEADERS([sys/reg.h], [], [])
dnl }}}

dnl {{{ Check functions
AC_CHECK_FUNCS([dup3 fchdir strsignal])
dnl }}}

dnl {{{ Check types
AC_CHECK_TYPES([struct pt_all_user_regs, struct ia64_fpreg],,,
			   [#include <sys/ptrace.h>])
dnl }}}

dnl {{{ Check whether getcwd(NULL, 0) allocates memory for result
AC_CACHE_CHECK([whether getcwd(NULL, 0) allocates memory for result],
			   [ac_cv_func_getcwd_null],
			   [AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    if (0 != chdir("/"))
        return EXIT_FAILURE;
    else {
        char *f = getcwd(NULL, 0);
        if (NULL != f && '/' == f[0] && '\0' == f[1])
            return EXIT_SUCCESS;
        else
            return EXIT_FAILURE;
    }
}
]])],
[ac_cv_func_getcwd_null=yes],
[ac_cv_func_getcwd_null=no])])
if test x"$ac_cv_func_getcwd_null" = x"yes"; then
	AC_DEFINE([HAVE_GETCWD_NULL], 1, [Define for getcwd(NULL, 0) that allocates memory])
fi
dnl }}}

dnl {{{ Check for usable /dev/null
AC_MSG_CHECKING([for /dev/null])
if ! test -c /dev/null; then
	AC_MSG_RESULT([NO!])
	AC_MSG_ERROR([/dev/null is not a special character file])
else
	AC_MSG_RESULT([yes])
fi

AC_CACHE_VAL(ac_cv_dev_null_usable)
AC_CACHE_CHECK([for usable /dev/null],
			   [ac_cv_dev_null_usable],
			   [AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DEVNULL "/dev/null"
#define TEST_STR "What exactly is a dream?"
#define TEST_LEN 25

int main(void) {
    int fd;
    char buf[TEST_LEN];

    fd = open(DEVNULL, O_WRONLY);
    if (0 > fd) {
        perror("open_wr");
        return EXIT_FAILURE;
    }

    if (0 > write(fd, TEST_STR, TEST_LEN)) {
        perror("write");
        return EXIT_FAILURE;
    }
    if (0 > close(fd)) {
        perror("close");
        return EXIT_FAILURE;
    }

    fd = open(DEVNULL, O_RDONLY);
    if (0 > fd) {
        perror("open_rd");
        return EXIT_FAILURE;
    }
    if (0 > read(fd, buf, TEST_LEN)) {
        perror("read");
        return EXIT_FAILURE;
    }
    if (0 == strncmp(buf, TEST_STR, TEST_LEN)) {
        fprintf(stderr, "strings match!\n");
        return EXIT_FAILURE;
    }
    if (0 > close(fd)) {
        perror("close");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
]])],
[ac_cv_dev_null_usable=yes],
[ac_cv_dev_null_usable=no])])
if test x"$ac_cv_dev_null_usable" = x"no"; then
	AC_MSG_ERROR([/dev/null fails to discard data])
fi
dnl }}}

dnl {{{ Check for usable /proc
AC_CACHE_CHECK([whether /proc/\$pid/cwd is usable],
			   [ac_cv_proc_cwd_usable],
			   [AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        exit(1);
    if (0 == pid) {
        pause();
        _exit(0);
    }
    else {
        char proc[32], path[PATH_MAX];
        char *cwd = NULL;
        ssize_t len;

        cwd = getcwd(cwd, PATH_MAX);
        if (NULL == cwd)
            goto fail;

        snprintf(proc, 32, "/proc/%i/cwd", pid);
        len = readlink(proc, path, PATH_MAX);
        if (0 > len)
            goto fail;
        else
            path[len] = '\0';
        if (0 != strncmp(cwd, path, PATH_MAX))
            goto fail;

        kill(pid, SIGKILL);
        exit(0);
fail:
        kill(pid, SIGKILL);
        exit(1);
    }
}
]])],
[ac_cv_proc_cwd_usable=yes],
[ac_cv_proc_cwd_usable=no])])
if test x"$ac_cv_proc_cwd_usable" = x"no"; then
	AC_MSG_ERROR([/proc fails to give current working directory information about pids])
fi

AC_CACHE_CHECK([whether /proc/\$pid/fd/\$dirfd is usable],
			   [ac_cv_proc_fd_usable],
			   [AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>

int main(void) {
    pid_t pid;
    char buf[32];
    int pfd[2];

    if (-1 == pipe(pfd))
        exit(1);

    pid = fork();
    if (0 == pid) {
        DIR *d = opendir("/dev");
        int dfd = dirfd(d);

        close(pfd[0]);

        snprintf(buf, 32, "%i", dfd);
        write(pfd[1], buf, 32);

        close(pfd[1]);

        pause();
        _exit(0);
    }
    else {
        int dfd, n;
        char proc[64], path[PATH_MAX];

        close(pfd[1]);

        if (0 > read(pfd[0], buf, 32))
            goto fail;

        close(pfd[0]);

        dfd = atoi(buf);
        snprintf(proc, 64, "/proc/%i/fd/%i", pid, dfd);
        n = readlink(proc, path, PATH_MAX);
        if (0 > n)
            goto fail;
        path[n] = '\0';

        if (0 != strncmp(path, "/dev", 5))
            goto fail;

        kill(pid, SIGKILL);
        exit(0);
fail:
        kill(pid, SIGKILL);
        exit(1);
    }
}
]])],
[ac_cv_proc_fd_usable=yes],
[ac_cv_proc_fd_usable=no])])
if test x"$ac_cv_proc_fd_usable" = x"no"; then
	AC_MSG_ERROR([/proc fails to give directory file descriptor information about pids])
fi

AC_CACHE_CHECK([whether /proc/self resolves to /proc/\$pid],
			   [ac_cv_have_proc_self],
			   [AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define PROC_SELF "/proc/self"

int main(void) {
    char buf[64], proc_pid[64];
    pid_t pid;

    if (0 > readlink(PROC_SELF, buf, 64)) {
        perror("readlink");
        return EXIT_FAILURE;
    }

    pid = getpid();
    snprintf(proc_pid, 64, "%i", pid);
    return (0 == strncmp(buf, proc_pid, 64)) ? EXIT_SUCCESS : EXIT_FAILURE;
}
]])],
[ac_cv_have_proc_self=yes],
[ac_cv_have_proc_self=no])])
if test x"$ac_cv_have_proc_self" = x"yes"; then
	AC_DEFINE([HAVE_PROC_SELF], 1, [Define for /proc/self])
fi
dnl }}}

dnl {{{ Check for large file support
AC_SYS_LARGEFILE
dnl }}}

dnl {{{ Extra CFLAGS
SYDBOX_CFLAGS=
WANTED_CFLAGS="-pedantic -Wall -W -Wextra -Wbad-function-cast -Wcast-align -Wcast-qual -Wfloat-equal -Wformat=2 -Wformat-security -Wformat-nonliteral -Winit-self -Winline -Wlogical-op -Wmissing-prototypes -Wmissing-declarations -Wmissing-format-attribute -Wmissing-noreturn -Wpointer-arith -Wredundant-decls -Wshadow -Wswitch-default -Wunused -Wvla"
for flag in $WANTED_CFLAGS ; do
	AX_CHECK_COMPILER_FLAGS([$flag], [SYDBOX_CFLAGS="$SYDBOX_CFLAGS $flag"],)
done
dnl }}}

dnl {{{ Check for gcc
AM_CONDITIONAL(GCC, test x"$GCC" = x"yes")
dnl }}}

dnl {{{ Profiling
AC_MSG_CHECKING([whether gprof symbols are wanted])
AC_ARG_ENABLE([gprof],
			  [AS_HELP_STRING([--enable-gprof],
							  [add gprof symbols (-pg) (for debugging)])],
			  SYDBOX_PROFILE="$enableval",
			  SYDBOX_PROFILE="no")
AC_MSG_RESULT([$SYDBOX_PROFILE])
if test x"$SYDBOX_PROFILE" = x"yes" ; then
	AX_CHECK_COMPILER_FLAGS([-pg],, AC_MSG_ERROR([your compiler doesn't support -pg flag]))
	SYDBOX_CFLAGS="$SYDBOX_CFLAGS -pg"
	LDFLAGS="$LDFLAGS -pg"
fi
AC_MSG_CHECKING([whether coverage testing should be enabled])
AC_ARG_ENABLE([gcov],
			  [AS_HELP_STRING([--enable-gcov],
							  [add gcov support (for debugging)])],
			  SYDBOX_COVERAGE="$enableval",
			  SYDBOX_COVERAGE="no")
AC_MSG_RESULT([$SYDBOX_COVERAGE])
if test x"$SYDBOX_COVERAGE" = x"yes" ; then
	AX_CHECK_COMPILER_FLAGS([-fprofile-arcs],, AC_MSG_ERROR([your compiler doesn't support -fprofile-arcs flag]))
	AX_CHECK_COMPILER_FLAGS([-ftest-coverage],, AC_MSG_ERROR([your compiler doesn't support -ftest-coverage flag]))
	SYDBOX_CFLAGS="$SYDBOX_CFLAGS -fprofile-arcs -ftest-coverage"
fi
AC_SUBST([SYDBOX_CFLAGS])
dnl }}}

dnl {{{ Check for libraries
GLIB_REQUIRED=2.0
PINKTRACE_REQUIRED=0.0.3

PKG_PROG_PKG_CONFIG([0.20.0])
PKG_CHECK_MODULES([glib], [glib-2.0 >= $GLIB_REQUIRED],,
				  AC_MSG_ERROR([sydbox requires glib-$GLIB_REQUIRED or newer]))
PKG_CHECK_MODULES([pinktrace], [pinktrace >= $PINKTRACE_REQUIRED],,
				  AC_MSG_ERROR([sydbox requires pinktrace-$PINKTRACE_REQUIRED or newer]))
dnl }}}

dnl {{{ Check for pinktrace's supported OS
AC_MSG_CHECKING([for pinktrace's supported operating system])
old_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS="${CPPFLAGS} ${pkg_cv_pinktrace_CFLAGS}"
AC_PREPROC_IFELSE([
#include <pinktrace/pink.h>
#ifndef PINKTRACE_LINUX
#error nope
#endif
],
	[AC_MSG_RESULT([ok])],
	[AC_MSG_ERROR([sydbox only supports Linux])])
CPPFLAGS="${old_CPPFLAGS}"
dnl }}}

dnl {{{ Check for pinktrace's supported bitness
AC_MSG_CHECKING([for pinktrace's supported bitness count])
old_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS="${CPPFLAGS} ${pkg_cv_pinktrace_CFLAGS}"
AC_PREPROC_IFELSE([
#include <pinktrace/pink.h>
#if PINKTRACE_BITNESS_COUNT_SUPPORTED < 2
#error nope
#endif
],
	[pinktrace_bitness_count_supported=2],
	[pinktrace_bitness_count_supported=1])
CPPFLAGS="${old_CPPFLAGS}"
AC_MSG_RESULT([$pinktrace_bitness_count_supported])
AM_CONDITIONAL(BITNESS_TWO, test x"$pinktrace_bitness_count_supported" = x"2")
dnl }}}

dnl {{{ Check for IPV6 support
AC_ARG_ENABLE([ipv6],
			  [AS_HELP_STRING([--disable-ipv6],
							  [disable IPV6 support])],
			  WANT_IPV6="$enableval",
			  WANT_IPV6="yes")
if test x"$WANT_IPV6" = x"yes" ; then
	AC_MSG_CHECKING([whether pinktrace was compiled with IPV6 support])
	old_CPPFLAGS="${CPPFLAGS}"
	CPPFLAGS="${CPPFLAGS} ${pkg_cv_pinktrace_CFLAGS}"
	AC_PREPROC_IFELSE([
	#include <pinktrace/pink.h>
	#if PINKTRACE_HAVE_IPV6 == 0
	#error nope
	#endif
	],
		[pinktrace_have_ipv6=yes],
		[pinktrace_have_ipv6=no])
	CPPFLAGS="${old_CPPFLAGS}"
	AC_MSG_RESULT([$pinktrace_have_ipv6])
	if test x"$pinktrace_have_ipv6" = x"no" ; then
		AC_MSG_ERROR([--enable-ipv6 requires pinktrace IPV6 support])
	fi
	AC_DEFINE([SYDBOX_HAVE_IPV6], 1, [Define for IPV6 support])
else
	AC_DEFINE([SYDBOX_HAVE_IPV6], 0, [Define for IPV6 support])
fi
AM_CONDITIONAL(WANT_IPV6, test x"$WANT_IPV6" = x"yes")
dnl }}}

dnl {{{ Check for Perl
AC_PATH_PROG([PERL], perl)
dnl }}}

dnl {{{ Use AM_SILENT_RULES if available
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
dnl }}}

dnl {{{ Output
AM_CONFIG_HEADER(config.h)
AC_OUTPUT(
	Makefile
	data/Makefile
	src/Makefile
	tests/Makefile
	tests/progtests/Makefile
	tests/unit/Makefile
	)
dnl }}}

