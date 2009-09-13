#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash
log="violation-$$.log"
clean_files+=( "$log" )

start_test "t39-magic-rmfilter-locked"
sydbox --lock -- bash <<EOF
[[ -e /dev/sydbox/rmfilter ]]
EOF
if [[ 0 == $? ]]; then
    die "/dev/sydbox/rmfilter exists"
fi
end_test

start_test "t39-magic-rmfilter-rm"
sydbox -- bash 2>"$log" <<EOF
[[ -e '/dev/sydbox/addfilter/${cwd}/*' ]]
[[ -e '/dev/sydbox/rmfilter/${cwd}/*' ]]
echo Oh Arnold Layne, its not the same > arnold.layne
EOF
if [[ 0 == $? ]]; then
    die "failed to remove filter using /dev/sydbox/rmfilter"
elif ! grep 'Reason:.*arnold\.layne' "$log" >/dev/null 2>&1; then
    echo "---" >&2
    cat "$log" >&2
    echo "---" >&2
    die "access violation not raised"
fi

