#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash

clean_files+=( "arnold.layne" )

start_test "t04-creat-deny"
sydbox -- ./t04_creat
if [[ 0 == $? ]]; then
    die "failed to deny creat"
elif [[ -f arnold.layne ]]; then
    die "file exists, failed to deny creat"
fi
end_test

start_test "t04-creat-write"
SYDBOX_WRITE="${cwd}" sydbox -- ./t04_creat
if [[ 0 != $? ]]; then
    die "failed to allow creat"
elif [[ ! -f arnold.layne ]]; then
    die "file doesn't exist, failed to allow creat"
fi
end_test

# Tests dealing with too long paths
fname="arnold.layne"
mkdir_long

start_test "t04-creat-toolong-deny"
sydbox -- ./t04_creat_toolong "$long_dir" "$fname"
if [[ 0 == $? ]]; then
    die "failed to deny creat"
elif lstat_long "$fname"; then
    die "file exists, failed to deny creat"
fi
end_test

start_test "t04-creat-toolong-write"
SYDBOX_WRITE="$cwd"/$long_dir sydbox -- ./t04_creat_toolong "$long_dir" "$fname"
if [[ 0 != $? ]]; then
    die "failed to allow creat"
elif ! lstat_long "$fname"; then
    die "file doesn't exist, failed to allow creat"
fi
end_test
