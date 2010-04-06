include $(top_srcdir)/tests/gtest.mk

AM_CFLAGS = $(glib_CFLAGS)

UNIT_TESTS = sydbox-utils children path trace net

# fake out libsydbox {{{
libsydbox_SOURCES = $(top_srcdir)/src/syd-children.c \
		    $(top_srcdir)/src/syd-config.c \
		    $(top_srcdir)/src/syd-net.c \
		    $(top_srcdir)/src/syd-path.c \
		    $(top_srcdir)/src/syd-trace.c \
		    $(top_srcdir)/src/syd-trace-util.c \
		    $(top_srcdir)/src/syd-utils.c
libsydbox_SOURCES+= $(top_srcdir)/src/syd-dispatch.h \
		    $(top_srcdir)/src/syd-dispatch-table.h
if I386
libsydbox_SOURCES+= $(top_srcdir)/src/syd-dispatch.c \
		    $(top_srcdir)/src/syd-trace-x86.c
endif
if X86_64
libsydbox_SOURCES+= $(top_srcdir)/src/syd-dispatch32.c \
		    $(top_srcdir)/src/syd-dispatch64.c \
		    $(top_srcdir)/src/syd-trace-x86_64.c
endif
if IA64
libsydbox_SOURCES+= $(top_srcdir)/src/syd-dispatch.c \
		    $(top_srcdir)/src/syd-trace-ia64.c
endif
if POWERPC
libsydbox_SOURCES+= $(top_srcdir)/src/syd-dispatch.c \
		    $(top_srcdir)/src/syd-trace-powerpc.c
endif
if POWERPC64
libsydbox_SOURCES+= $(top_srcdir)/src/syd-dispatch.c \
		    $(top_srcdir)/src/syd-trace-powerpc.c
endif

AM_CFLAGS += -DDATADIR="\"$(datadir)\"" -DSYSCONFDIR="\"$(sysconfdir)\"" -I$(top_srcdir)/src
# }}}

TESTS += $(UNIT_TESTS)

noinst_PROGRAMS = $(UNIT_TESTS)

sydbox_utils_SOURCES = $(libsydbox_SOURCES) test-sydbox-utils.c
sydbox_utils_LDADD = $(glib_LIBS)

children_SOURCES = $(libsydbox_SOURCES) test-children.c
children_LDADD = $(glib_LIBS)

path_SOURCES = $(libsydbox_SOURCES) test-path.c
path_LDADD = $(glib_LIBS)

trace_SOURCES = $(libsydbox_SOURCES) test-helpers.h test-trace.c
trace_LDADD = $(glib_LIBS)

net_SOURCES= $(libsydbox_SOURCES) test-helpers.h test-net.c
net_LDADD= $(glib_LIBS)
