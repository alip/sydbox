#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash

start_test "t43-exit-code-single"
sydbox -- ./t43_exit_code_single 127
if [[ 127 != $? ]]; then
    die "wrong exit code for single: ${?}"
fi
end_test

expected=$(( 128 + $(kill -l TERM) ))
start_test "t43-exit-code-signal"
sydbox -- ./t43_exit_code_signal &
pid=$!
kill -TERM $pid
wait $pid
if [[ $expected != $? ]]; then
    die "wrong exit code for TERM signal: ${?}, expected: ${expected}"
fi
end_test

start_test "t43-exit-code-many"
sydbox -- ./t43_exit_code_many 127 128 1
if [[ 127 != $? ]]; then
    die "wrong exit code for many: ${?}"
fi
end_test
