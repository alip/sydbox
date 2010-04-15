#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2010 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash

start_test "t49-bind-unsupported-family"
sydbox -N -B -- ./t49_bind_unsupported_family
if [[ 0 != $? ]]; then
    die "bind call with unsupported family failed (segfault?)"
fi
end_test
