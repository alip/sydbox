#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2010 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash

# By default sydbox allows access to /proc/$pid
start_test "t51-allow-proc-pid"
sydbox -- ./t51_allow_proc_pid
if [[ 0 != $? ]]; then
    die "access to /proc/\$pid not allowed"
fi
end_test
