#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t01-chmod-deny"
sydbox -- ./t01_chmod
if [[ 0 == $? ]]; then
    die "failed to deny chmod"
fi
perms=$(ls -l arnold.layne | cut -d' ' -f1)
if [[ "${perms}" != '-rw-r--r--' ]]; then
    die "permissions changed, failed to deny chmod"
fi
end_test

start_test "t01-chmod-write"
SYDBOX_WRITE="${cwd}" sydbox -- ./t01_chmod
if [[ 0 != $? ]]; then
    die "failed to allow chmod"
fi
perms=$(ls -l arnold.layne | cut -d' ' -f1)
if [[ "${perms}" != '----------' ]]; then
    die "write didn't allow access"
fi
end_test

# Tests dealing with too long paths
tmpfile="$(mkstemp_long)"
if [[ -z "$tmpfile" ]]; then
    say skip "failed to create temporary file, skipping test (no perl?)"
    exit 0
fi

start_test "t01-chmod-deny-toolong"
sydbox -- ./t01_chmod_toolong "$long_dir" "$tmpfile"
if [[ 0 == $? ]]; then
    die "failed to deny chmod"
fi
end_test

start_test "t01-chmod-allow-toolong"
SYDBOX_WRITE="$cwd"/$long_dir sydbox -- ./t01_chmod_toolong "$long_dir" "$tmpfile"
if [[ 0 != $? ]]; then
    die "failed to allow chmod"
fi
perms=$(perm_long "$tmpfile")
if [[ -z "$perms" ]]; then
    say skip "failed to get permissions of the file, skipping test"
    exit 0
elif [[ "$perms" != 0 ]]; then
    die "write didn't allow access"
fi
end_test
