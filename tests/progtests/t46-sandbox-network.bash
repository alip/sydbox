#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2010 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash
clean_files+=( "$bind_socket" )

start_test "t46-sandbox-network-allow-bind-unix"
sydbox -N -M allow -- ./t46_sandbox_network_bind_unix "$bind_socket"
if [[ 0 != $? ]]; then
    die "Failed to allow binding to UNIX socket"
fi
end_test

start_test "t46-sandbox-network-allow-bind-tcp"
sydbox -N -M allow -- ./t46_sandbox_network_bind_tcp $bind_port
if [[ 0 != $? ]]; then
    die "Failed to allow binding to TCP socket"
fi
end_test

start_test "t46-sandbox-network-allow-connect"
end_test

start_test "t46-sandbox-network-allow-sendto"
end_test

start_test "t46-sandbox-network-deny-bind-unix"
end_test

start_test "t46-sandbox-network-deny-bind-tcp"
end_test

start_test "t46-sandbox-network-deny-connect"
end_test

start_test "t46-sandbox-network-deny-sendto"
end_test

start_test "t46-sandbox-network-deny-allow-whitelisted-bind-unix"
end_test

start_test "t46-sandbox-network-deny-allow-whitelisted-bind-tcp"
end_test

start_test "t46-sandbox-network-deny-allow-whitelisted-connect"
end_test

start_test "t46-sandbox-network-deny-allow-whitelisted-sendto"
end_test

start_test "t46-sandbox-network-local-allow-bind-unix"
end_test

start_test "t46-sandbox-network-local-allow-bind-tcp"
end_test

start_test "t46-sandbox-network-local-allow-connect"
end_test

start_test "t46-sandbox-network-local-allow-sendto"
end_test

start_test "t46-sandbox-network-local-deny-remote-bind-tcp"
end_test

start_test "t46-sandbox-network-local-deny-remote-connect"
end_test

start_test "t46-sandbox-network-local-deny-remote-sendto"
end_test

start_test "t46-sandbox-network-local-allow-whitelisted-remote-bind-tcp"
end_test

start_test "t46-sandbox-network-local-allow-whitelisted-remote-connect"
end_test

start_test "t46-sandbox-network-local-allow-whitelisted-remote-sendto"
end_test

start_test "t46-sandbox-network-local-restrict_connect-unix"
end_test

start_test "t46-sandbox-network-local-restrict_connect-tcp"
end_test
