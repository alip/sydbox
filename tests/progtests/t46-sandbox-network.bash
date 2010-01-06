#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2010 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash
bind_socket="$cwd"/sydbox.sock
bind_port=23456
clean_files+=( "$bind_socket" )

start_test "t46-sandbox-network-allow-bind-unix"
sydbox -N -M allow -- ./t46_sandbox_network_bind_unix "$bind_socket"
if [[ 0 != $? ]]; then
    die "Failed to allow binding to UNIX socket"
fi
end_test

start_test "t46-sandbox-network-allow-bind-tcp"
sydbox -N -M allow -- ./t46_sandbox_network_bind_tcp '127.0.0.1' $bind_port
if [[ 0 != $? ]]; then
    die "Failed to allow binding to TCP socket"
fi
end_test

start_test "t46-sandbox-network-allow-connect-tcp"
# Start a TCP server in the background.
fail="tcp-server-failed"
rm -f "$fail"
clean_files+=( "$fail" )
tcp_server '127.0.0.1' $bind_port "$fail" &
pid=$!
sleep 1
if [[ -e "$fail" ]]; then
    say skip "Failed to start TCP server: $(< $fail)"
    say skip "Skipping test"
else
    sydbox -N -M allow -- ./t46_sandbox_network_connect_tcp '127.0.0.1' $bind_port
    if [[ 0 != $? ]]; then
        kill $pid
        die "Failed to allow connect to a TCP server"
    else
        wait $pid
    fi
fi
end_test

start_test "t46-sandbox-network-allow-connect-unix"
# Start a Unix server in the background.
fail="unix-server-failed"
clean_files+=( "$fail" )
unix_server "$bind_socket" "$fail" &
pid=$!
sleep 1
if [[ -e "$fail" ]]; then
    say skip "Failed to start Unix server: $(< $fail)"
    say skip "Skipping test"
else
    sydbox -N -M allow -- ./t46_sandbox_network_connect_unix "$bind_socket"
    if [[ 0 != $? ]]; then
        kill $pid
        die "Failed to allow connect to a Unix server"
    else
        wait $pid
    fi
fi
end_test

start_test "t46-sandbox-network-allow-sendto (TODO)"
end_test

unlink "$bind_socket" 2>/dev/null
start_test "t46-sandbox-network-deny-bind-unix"
sydbox -N -M deny -- ./t46_sandbox_network_bind_unix_deny "$bind_socket"
if [[ 0 == $? ]]; then
    die "Failed to deny bind to a UNIX socket"
fi
end_test

start_test "t46-sandbox-network-deny-bind-tcp"
sydbox -N -M deny -- ./t46_sandbox_network_bind_tcp_deny '127.0.0.1' $bind_port
if [[ 0 == $? ]]; then
    die "Failed to deny bind to a TCP socket"
fi
end_test

start_test "t46-sandbox-network-deny-connect-tcp"
# Start a TCP server in the background.
fail="tcp-server-failed"
rm -f "$fail"
clean_files+=( "$fail" )
tcp_server '127.0.0.1' $bind_port "$fail" &
pid=$!
sleep 1
if [[ -e "$fail" ]]; then
    say skip "Failed to start TCP server: $(< $fail)"
    say skip "Skipping test"
else
    sydbox -N -M deny -- ./t46_sandbox_network_connect_tcp_deny '127.0.0.1' $bind_port
    if [[ 0 == $? ]]; then
        kill $pid
        die "Failed to deny connect to a TCP server"
    else
        wait $pid
    fi
fi
end_test

start_test "t46-sandbox-network-deny-connect-unix"
# Start a Unix server in the background.
fail="unix-server-failed"
clean_files+=( "$fail" )
unix_server "$bind_socket" "$fail" &
pid=$!
sleep 1
if [[ -e "$fail" ]]; then
    say skip "Failed to start Unix server: $(< $fail)"
    say skip "Skipping test"
else
    sydbox -N -M deny -- ./t46_sandbox_network_connect_unix_deny "$bind_socket"
    if [[ 0 == $? ]]; then
        kill $!
        die "Failed to deny connect to a Unix server"
    else
        wait $!
    fi
fi
end_test

start_test "t46-sandbox-network-deny-sendto (TODO)"
end_test

unlink "$bind_socket" 2>/dev/null
start_test "t46-sandbox-network-deny-allow-whitelisted-bind-unix"
SYDBOX_NET_WHITELIST=unix://"$bind_socket" \
sydbox -N -M deny -- ./t46_sandbox_network_bind_unix "$bind_socket"
if [[ 0 != $? ]]; then
    die "Failed to allow bind to Unix socket by whitelisting"
fi
end_test

start_test "t46-sandbox-network-deny-allow-whitelisted-bind-tcp"
SYDBOX_NET_WHITELIST=inet://127.0.0.1:$bind_port \
sydbox -N -M deny -- ./t46_sandbox_network_bind_tcp 127.0.0.1 $bind_port
if [[ 0 != $? ]]; then
    die "Failed to allow bind to TCP socket by whitelisting"
fi
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
