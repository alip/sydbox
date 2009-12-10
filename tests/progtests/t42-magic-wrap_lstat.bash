#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash

start_test "t42-magic-wrap_lstat-locked1"
sydbox --lock -- bash << EOF
[[ -e /dev/sydbox/wrap/lstat ]]
EOF
if [[ 0 == $? ]]; then
    die "/dev/sydbox/wrap/lstat exists"
fi
end_test

start_test "t42-magic-wrap_lstat-locked2"
sydbox --lock -- bash << EOF
[[ -e /dev/sydbox/nowrap/lstat ]]
EOF
if [[ 0 == $? ]]; then
    die "/dev/sydbox/nowrap/lstat exists"
fi
end_test

fname="vegetable.man"
write_long "$fname" "And all the lot is what I've got"

start_test "t42-magic-wrap_lstat-nowrap-commandline"
sydbox --nowrap-lstat -- ./t42_magic_wrap_lstat "$long_dir" "$fname"
if [[ 0 == $? ]]; then
    die "failed to disable lstat() wrapper using --nowrap-lstat"
fi
data="$(read_long $fname)"
if [[ -z "$data" ]]; then
    die "file truncated, failed to disable lstat() wrapper using --nowrap-lstat"
fi
end_test

start_test "t42-magic-wrap_lstat-nowrap-devsydbox"
sydbox -- bash <<EOF
[[ -e /dev/sydbox/nowrap/lstat ]]
./t42_magic_wrap_lstat "$long_dir" "$fname"
EOF
if [[ 0 == $? ]]; then
    die "failed to disable lstat() wrapper using /dev/sydbox/nowrap/lstat"
fi
data="$(read_long $fname)"
if [[ -z "$data" ]]; then
    die "file truncated, failed to disable lstat() wrapper using /dev/sydbox/nowrap/lstat"
fi
end_test

start_test "t42-magic-wrap_lstat-nowrap-devsydbox-reenable"
sydbox -- bash <<EOF
[[ -e /dev/sydbox/nowrap/lstat ]]
[[ -e /dev/sydbox/wrap/lstat ]]
[[ -e /dev/sydbox/nowrap/lstat ]]
./t42_magic_wrap_lstat "$long_dir" "$fname"
EOF
if [[ 0 == $? ]]; then
    die "failed to disable lstat() wrapper using /dev/sydbox/nowrap/lstat"
fi
data="$(read_long $fname)"
if [[ -z "$data" ]]; then
    die "file truncated, failed to disable lstat() wrapper using /dev/sydbox/nowrap/lstat"
fi
end_test
