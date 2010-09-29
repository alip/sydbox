include $(top_srcdir)/tests/gtest.mk

AM_CFLAGS= $(glib_CFLAGS) $(pinktrace_CFLAGS)

UNIT_TESTS= sydbox-utils path children net

# fake out libsydbox {{{
libsydbox_SOURCES = $(top_srcdir)/src/syd-children.c \
		    $(top_srcdir)/src/syd-config.c \
		    $(top_srcdir)/src/syd-net.c \
		    $(top_srcdir)/src/syd-path.c \
		    $(top_srcdir)/src/syd-pink.c \
		    $(top_srcdir)/src/syd-utils.c
if BITNESS_TWO
libsydbox_SOURCES+= $(top_srcdir)/src/syd-dispatch32.c \
		    $(top_srcdir)/src/syd-dispatch64.c
else
libsydbox_SOURCES+= $(top_srcdir)/src/syd-dispatch.c
endif # BITNESS_TWO

AM_CFLAGS+= -DDATADIR="\"$(datadir)\"" -DSYSCONFDIR="\"$(sysconfdir)\"" -I$(top_srcdir)/src
# }}}

TESTS+= $(UNIT_TESTS)

noinst_PROGRAMS= $(UNIT_TESTS)

sydbox_utils_SOURCES= $(libsydbox_SOURCES) test-sydbox-utils.c
sydbox_utils_LDADD= $(glib_LIBS) $(pinktrace_LIBS)

children_SOURCES= $(libsydbox_SOURCES) test-children.c
children_LDADD= $(glib_LIBS) $(pinktrace_LIBS)

path_SOURCES= $(libsydbox_SOURCES) test-path.c
path_LDADD= $(glib_LIBS) $(pinktrace_LIBS)

net_SOURCES= $(libsydbox_SOURCES) test-helpers.h test-net.c
net_LDADD= $(glib_LIBS) $(pinktrace_LIBS)
