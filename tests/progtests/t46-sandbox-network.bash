#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2010 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash
bind_socket="$cwd"/sydbox.sock
bind_port=23456
clean_files+=( "$bind_socket" )

unlink "$bind_socket" 2>/dev/null
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

unlink "$bind_socket" 2>/dev/null
start_test "t46-sandbox-network-local-allow-bind-unix"
sydbox -N -M "local" -- ./t46_sandbox_network_bind_unix "$bind_socket"
if [[ 0 != $? ]]; then
    die "Failed to allow binding to UNIX socket in local mode"
fi
end_test

start_test "t46-sandbox-network-local-allow-bind-tcp"
sydbox -N -M "local" -- ./t46_sandbox_network_bind_tcp '127.0.0.1' $bind_port
if [[ 0 != $? ]]; then
    die "Failed to allow binding to TCP socket in local mode"
fi
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

# Start a TCP server in background.
has_tcp=false
fail="tcp-server-failed"
clean_files+=( "$fail" )
start_test "t46-sandbox-network-allow-connect-tcp"
tcp_server 127.0.0.1 $bind_port "$fail" &
tcp_pid=$!
sleep 1
if [[ -e "$fail" ]]; then
    say skip "Failed to start TCP server: $(< $fail)"
else
    has_tcp=true
fi

# Start a Unix server in background.
has_unix=false
fail="unix-server-failed"
clean_files+=( "$fail" )
unix_server "$bind_socket" "$fail" &
unix_pid=$!
sleep 1
if [[ -e "$fail" ]]; then
    say skip "Failed to start Unix server: $(< $fail)"
else
    has_unix=true
fi

shutdown() {
    send_tcp_server 127.0.0.1 $bind_port "QUIT"
    send_unix_server "$bind_socket" "QUIT"
    wait $tcp_pid $unix_pid
}
trap 'shutdown' EXIT

start_test "t46-sandbox-network-allow-connect-unix"
if $has_unix; then
    sydbox -N -M allow -- ./t46_sandbox_network_connect_unix "$bind_socket"
    if [[ 0 != $? ]]; then
        die "Failed to allow connect to a Unix socket"
    fi
else
    say skip "No Unix server, skipping test"
fi
end_test

start_test "t46-sandbox-network-allow-connect-tcp"
if $has_tcp; then
    sydbox -N -M allow -- ./t46_sandbox_network_connect_tcp '127.0.0.1' $bind_port
    if [[ 0 != $? ]]; then
        die "Failed to allow connect to a TCP socket"
    fi
else
    say skip "No TCP server, skipping test"
fi
end_test

start_test "t46-sandbox-network-allow-sendto (TODO)"
end_test

start_test "t46-sandbox-network-deny-connect-unix"
if $has_unix; then
    sydbox -N -M deny -- ./t46_sandbox_network_connect_unix_deny "$bind_socket"
    if [[ 0 == $? ]]; then
        die "Failed to deny connect to a Unix server"
    fi
else
    say skip "No Unix server, skipping test"
fi
end_test

start_test "t46-sandbox-network-deny-connect-tcp"
if $has_tcp; then
    sydbox -N -M deny -- ./t46_sandbox_network_connect_tcp_deny '127.0.0.1' $bind_port
    if [[ 0 == $? ]]; then
        die "Failed to deny connect to a TCP server"
    fi
else
    say skip "No TCP server, skipping test"
fi
end_test

start_test "t46-sandbox-network-deny-sendto (TODO)"
end_test

start_test "t46-sandbox-network-deny-allow-whitelisted-connect-unix"
if $has_unix; then
    SYDBOX_NET_WHITELIST=unix://"$bind_socket" \
    sydbox -N -M deny -- ./t46_sandbox_network_connect_unix "$bind_socket"
    if [[ 0 != $? ]]; then
        die "Failed to allow connect to a Unix server by whitelisting"
    fi
else
    say skip "No Unix server, skipping test"
fi
end_test

start_test "t46-sandbox-network-deny-allow-whitelisted-connect-tcp"
if $has_tcp; then
    SYDBOX_NET_WHITELIST=inet://127.0.0.1:$bind_port \
    sydbox -N -M deny -- ./t46_sandbox_network_connect_tcp '127.0.0.1' $bind_port
    if [[ 0 != $? ]]; then
        die "Failed to allow connect to a TCP server by whitelisting"
    fi
else
    say skip "No TCP server, skipping test"
fi
end_test

start_test "t46-sandbox-network-deny-allow-whitelisted-sendto (TODO)"
end_test

start_test "t46-sandbox-network-local-allow-connect-unix"
if $has_unix; then
    sydbox -N -M 'local' -- ./t46_sandbox_network_connect_unix "$bind_socket"
    if [[ 0 != $? ]]; then
        die "Failed to allow connect to a Unix socket in local mode"
    fi
else
    say skip "No Unix server, skipping test"
fi
end_test

start_test "t46-sandbox-network-local-allow-connect-tcp"
if $has_tcp; then
    sydbox -N -M 'local' -- ./t46_sandbox_network_connect_tcp '127.0.0.1' $bind_port
    if [[ 0 != $? ]]; then
        die "Failed to allow connect to a TCP socket in local mode"
    fi
else
    say skip "No TCP server, skipping test"
fi
end_test

start_test "t46-sandbox-network-local-allow-sendto (TODO)"
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
