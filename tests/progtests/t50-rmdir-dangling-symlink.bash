#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2010 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t50-rmdir-dangling-symlink"
SYDBOX_WRITE="${cwd}" \
sydbox -- ./t50_rmdir_dangling_symlink "i.am.the.eggman"
if [[ 0 != $? ]]; then
    die "rmdir didn't set errno to ENOTDIR for dangling symlink"
fi
end_test
