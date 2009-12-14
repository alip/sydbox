#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash

start_test "t45-sandbox-exec-first-exec"
sydbox -E -- ./t45_sandbox_exec_success
if [[ 0 != $? ]]; then
    die "first exec is sandboxed!"
fi
end_test

start_test "t45-sandbox-exec-try-exec"
sydbox -E -- ./t45_sandbox_exec_try_exec
if [[ 0 == $? ]]; then
    die "exec isn't sandboxed!"
fi
end_test
