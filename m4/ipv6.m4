dnl vim: set sw=4 sts=4 ts=4 noet ft=config foldmethod=marker foldmarker={{{,}}} :
dnl Based in part upon netmap-5.00

AC_DEFUN([APR_CHECK_SOCKADDR_IN6],[
AC_CACHE_CHECK(for sockaddr_in6, ac_cv_define_sockaddr_in6,[
AC_TRY_COMPILE([
#include <sys/types.h>
#include <netinet/in.h>
],[
struct sockaddr_in6 sa;
],[
    ac_cv_define_sockaddr_in6=yes
],[
    ac_cv_define_sockaddr_in6=no
])
])

if test "$ac_cv_define_sockaddr_in6" = "yes"; then
  have_sockaddr_in6=1
else
  have_sockaddr_in6=0
fi
])

AC_DEFUN([CHECK_AF_INET6_DEFINE],[
AC_CACHE_CHECK(for AF_INET6 definition, ac_cv_define_af_inet6,[
AC_TRY_COMPILE([
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
],[
int af = AF_INET6;
],[
    ac_cv_define_af_inet6=yes
],[
    ac_cv_define_af_inet6=no
])
])

if test "$ac_cv_define_af_inet6" = "yes"; then
  have_af_inet6=1
else
  have_af_inet6=0
fi
])
