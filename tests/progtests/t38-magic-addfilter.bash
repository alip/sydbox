#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash
log="violation-$$.log"
clean_files+=( "$log" )

start_test "t38-magic-addfilter-locked"
sydbox --lock -- bash <<EOF
[[ -e /dev/sydbox/addfilter ]]
EOF
if [[ 0 == $? ]]; then
    die "/dev/sydbox/addfilter exists"
fi
end_test

start_test "t38-magic-addfilter-add"
sydbox -- bash 2>"$log" <<EOF
[[ -e '/dev/sydbox/addfilter/${cwd}/*' ]]
echo Oh Arnold Layne, its not the same > arnold.layne
EOF
if [[ 0 == $? ]]; then
    die "filter allowed access"
elif grep 'Reason:.*arnold\.layne' "$log" >/dev/null 2>&1; then
    echo "---" >&2
    cat "$log" >&2
    echo "---" >&2
    die "access violation raised"
fi

