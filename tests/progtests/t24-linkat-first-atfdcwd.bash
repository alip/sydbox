#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

clean_files+=( "arnold.layne.hard" )

start_test "t24-linkat-first-atfdcwd-deny"
sydbox -- ./t24_linkat_first_atfdcwd
if [[ 0 == $? ]]; then
    die "failed to deny linkat"
elif [[ -f arnold.layne.hard ]]; then
    die "file exists, failed to deny linkat"
fi
end_test

start_test "t24-linkat-first-atfdcwd-write"
SYDBOX_WRITE="${cwd}" sydbox -- ./t24_linkat_first_atfdcwd
if [[ 0 != $? ]]; then
    die "failed to allow linkat"
elif [[ ! -f arnold.layne.hard ]]; then
    die "file doesn't exist, failed to allow linkat"
fi
end_test
