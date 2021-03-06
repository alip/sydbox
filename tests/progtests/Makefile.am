CLEANFILES = *.log test-lib.bash

# The size of the long directory tree we create
DIR_COUNT=64

AM_CFLAGS= -DDATADIR=\"$(datadir)\" -DSYSCONFDIR=\"$(sysconfdir)\" \
	   -DGIT_HEAD=\"$(GIT_HEAD)\" -DDIR_COUNT=$(DIR_COUNT) \
	   $(glib_CFLAGS) @SYDBOX_CFLAGS@

TESTS = t01-chmod.bash t02-chown.bash t03-open.bash t04-creat.bash t05-lchown.bash \
	t06-link.bash t07-mkdir.bash t08-mknod.bash t09-access.bash \
	t10-rename-first.bash t11-rename-second.bash t12-rmdir.bash t13-symlink.bash \
	t14-truncate.bash t15-mount.bash t16-umount.bash t17-umount2.bash t18-utime.bash \
	t19-unlink.bash t20-renameat-first-atfdcwd.bash t21-renameat-first.bash \
	t22-renameat-second-atfdcwd.bash t23-renameat-second.bash t24-linkat-first-atfdcwd.bash \
	t25-linkat-first.bash t26-linkat-second-atfdcwd.bash t27-linkat-second.bash \
	t28-symlinkat-atfdcwd.bash t29-symlinkat.bash t30-fchmodat-atfdcwd.bash t31-fchmodat.bash \
	t32-magic-onoff.bash t33-magic-enabled.bash t34-magic-lock.bash t35-magic-exec_lock.bash \
	t36-magic-write.bash t37-magic-unwrite.bash t38-magic-addfilter.bash t39-magic-rmfilter.bash \
	t40-openat-ebadf.bash t41-openat-fileno.bash t42-magic-wrap_lstat.bash t43-exit-code.bash \
	t44-bad-open.bash t45-sandbox-exec.bash t46-sandbox-network.bash
if WANT_IPV6
TESTS += t47-sandbox-network-ipv6.bash
endif
TESTS+= t48-sandbox-network-bindzero.bash t49-bind-unsupported-family.bash \
	t50-rmdir-dangling-symlink.bash t51-allow-proc-pid.bash

EXTRA_DIST= $(TESTS)

check_PROGRAMS = test-lib.bash t01_chmod t01_chmod_toolong t02_chown t02_chown_toolong \
		 t03_open t03_open_toolong t04_creat t04_creat_toolong t05_lchown \
		 t06_link t07_mkdir t07_mkdir_toolong t08_mknod t08_mknod_toolong t09_access \
		 t10_rename_first t11_rename_second t12_rmdir t12_rmdir_toolong t13_symlink \
		 t13_symlink_toolong t14_truncate t14_truncate_toolong t15_mount t16_umount \
		 t17_umount2 t18_utime t19_unlink t19_unlink_toolong t20_renameat_first_atfdcwd \
		 t21_renameat_first t22_renameat_second_atfdcwd t23_renameat_second \
		 t24_linkat_first_atfdcwd t25_linkat_first t26_linkat_second_atfdcwd t27_linkat_second \
		 t28_symlinkat_atfdcwd t29_symlinkat t30_fchmodat_atfdcwd t31_fchmodat \
		 t32_magic_onoff_set_on t32_magic_onoff_set_off t32_magic_onoff_check_off \
		 t32_magic_onoff_check_on t40_openat_ebadf t41_openat_fileno t42_magic_wrap_lstat \
		 t43_exit_code_single t43_exit_code_signal t43_exit_code_many t44_bad_open \
		 t45_sandbox_exec_success t45_sandbox_exec_try_exec t46_sandbox_network_bind_unix \
		 t46_sandbox_network_bind_unix_abstract t46_sandbox_network_bind_tcp \
		 t46_sandbox_network_connect_tcp t46_sandbox_network_connect_unix \
		 t46_sandbox_network_bind_unix_deny t46_sandbox_network_bind_unix_abstract_deny \
		 t46_sandbox_network_bind_tcp_deny t46_sandbox_network_connect_tcp_deny \
		 t46_sandbox_network_connect_unix_deny t46_sandbox_network_bind_connect_unix \
		 t46_sandbox_network_bind_connect_tcp
if WANT_IPV6
check_PROGRAMS+= t47_sandbox_network_bind_ipv6 t47_sandbox_network_bind_ipv6_deny
endif
check_PROGRAMS+= t48_sandbox_network_bindzero_connect_tcp t48_sandbox_network_bindzero_dup_connect_tcp \
		 t48_sandbox_network_bindzero_dup2_connect_tcp t48_sandbox_network_bindzero_dup3_connect_tcp \
		 t48_sandbox_network_bindzero_fdupfd_connect_tcp t49_bind_unsupported_family \
		 t50_rmdir_dangling_symlink t51_allow_proc_pid

test_lib_bash_SOURCES= test-lib.bash.in

test-lib.bash: test-lib.bash.in
	$(SED) \
		-e "s:@TOP_BUILDDIR@:$(top_builddir):" \
		-e "s:@PERL_PATH@:$(PERL):" \
		-e "s:@DIR_COUNT@:$(DIR_COUNT):" \
		$< > $@

check-valgrind:
	SYDBOX_RUN_UNDER_VALGRIND=1 $(MAKE) check

.PHONY: check-valgrind
