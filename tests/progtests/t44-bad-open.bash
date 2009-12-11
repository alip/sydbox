#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

# Regression test for bug 213
# https://bugs.exherbo.org/show_bug.cgi?id=213

no_create_files=1
. test-lib.bash

start_test "t44-bad-open"
sydbox -- ./t44_bad_open
if [[ 0 != $? ]]; then
    die "bad open call causes sydbox to fail"
fi
end_test
