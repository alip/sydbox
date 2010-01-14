#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2010 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash

start_test "t48-sandbox-network-bindzero-whitelist"
SYDBOX_NET_WHITELIST_BIND=inet://127.0.0.1@0 \
sydbox -N -B -- ./t48_sandbox_network_bindzero_connect_tcp 127.0.0.1
if [[ 0 != $? ]]; then
    die "Failed to whitelist bindzero"
fi
end_test

start_test "t48-sandbox-network-bindzero-dup"
SYDBOX_NET_WHITELIST_BIND=inet://127.0.0.1@0 \
sydbox -N -B -- ./t48_sandbox_network_bindzero_dup_connect_tcp 127.0.0.1
if [[ 0 != $? ]]; then
    die "Failed to whitelist bindzero address after dup()"
fi
end_test

start_test "t48-sandbox-network-bindzero-dup2"
SYDBOX_NET_WHITELIST_BIND=inet://127.0.0.1@0 \
sydbox -N -B -- ./t48_sandbox_network_bindzero_dup2_connect_tcp 127.0.0.1
if [[ 0 != $? ]]; then
    die "Failed to whitelist bindzero address after dup2()"
fi
end_test

start_test "t48-sandbox-network-bindzero-dup3"
SYDBOX_NET_WHITELIST_BIND=inet://127.0.0.1@0 \
sydbox -N -B -- ./t48_sandbox_network_bindzero_dup3_connect_tcp 127.0.0.1
if [[ 0 != $? ]]; then
    die "Failed to whitelist bindzero address after dup3()"
fi
end_test
