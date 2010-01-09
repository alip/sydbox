#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2010 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash
bind_port=23456

start_test "t47-sandbox-network-ipv6-deny-bind"
sydbox -N -- ./t47_sandbox_network_bind_ipv6_deny '::1' $bind_port
if [[ 0 == $? ]]; then
    die "Failed to deny binding to an IPV6 address"
fi
end_test

start_test "t47-sandbox-network-ipv6-deny-whitelisted-bind"
SYDBOX_NET_WHITELIST_BIND=inet6://::1@$bind_port \
sydbox -N -- ./t47_sandbox_network_bind_ipv6 '::1' $bind_port
if [[ 0 != $? ]]; then
    die "Failed to allow binding to an IPV6 address by whitelisting"
fi
end_test

# TODO: Write test cases for connect() as well.
