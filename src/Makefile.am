CLEANFILES= gmon.out *.gcda *.gcno *.gcov
EXTRA_DIST=

DEFS+= -DDATADIR=\"$(datadir)\" -DSYSCONFDIR=\"$(sysconfdir)\" \
       -DGIT_HEAD=\"$(GIT_HEAD)\"
AM_CFLAGS= $(glib_CFLAGS) $(pinktrace_CFLAGS) @SYDBOX_CFLAGS@
bin_PROGRAMS = sydbox
noinst_HEADERS= syd-children.h syd-config.h syd-context.h syd-flags.h \
		syd-log.h syd-log.h syd-loop.h syd-net.h syd-path.h \
		syd-pink.h syd-proc.h syd-syscall.h \
		syd-wrappers.h syd-utils.h
sydbox_SOURCES = syd-children.c syd-config.c syd-context.c syd-log.c \
		 syd-loop.c syd-net.c syd-pink.c syd-path.c syd-proc.c \
		 syd-syscall.c syd-utils.c syd-wrappers.c syd-main.c
sydbox_LDADD= $(glib_LIBS) $(pinktrace_LIBS)

noinst_HEADERS+= syd-dispatch.h syd-dispatch-table.h
if BITNESS_TWO
sydbox_SOURCES+= syd-dispatch32.c syd-dispatch64.c
else
sydbox_SOURCES+= syd-dispatch.c
endif # BITNESS_TWO

SPARSE=sparse
SPARSE_CPPFLAGS= $(DEFAULT_INCLUDES) \
		 -Wbitwise -Wcast-to-as -Wdefault-bitfield-sign \
		 -Wparen-string -Wptr-subtraction-blows \
		 -Wreturn-void -Wshadow -Wtypesign -Wundef \
		 -I$(shell $(CC) -print-file-name=include) \
		 -I$(shell $(CC) -print-file-name=include-fixed)

# Fix this flag for your architecture!
SPARSE_CPPFLAGS+= -D__x86_64__=1
# SPARSE_CPPFLAGS+= -D__i386__=1
# SPARSE_CPPFLAGS+= -D__ia64__=1
# SPARSE_CPPFLAGS+= -D__powerpc__=1
# SPARSE_CPPFLAGS+= -D__powerpc64__=1

sparse-check:
	for src in $(sydbox_SOURCES); \
	do \
		$(SPARSE) $(DEFS) $(SPARSE_CPPFLAGS) $(glib_CFLAGS) $(pinktrace_CFLAGS) $$src || exit 1; \
	done

.PHONY: sparse-check
