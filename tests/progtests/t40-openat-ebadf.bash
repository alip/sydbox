#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Elias Pipping <elias@pipping.org>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash

start_test "t40-openat-ebadf"
sydbox -- ./t40_openat_ebadf
if [[ 0 != $? ]]; then
    die "openat did not set errno to EBADF"
fi
end_test
