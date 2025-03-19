#!/usr/bin/env bash

TCPROXY_LISTEN_PROTO=6
TCPROXY_LISTEN_PORT=22755
TCPROXY_REMOTE_PROTO=4
TCPROXY_REMOTE_ADDR=178.209.113.50
TCPROXY_REMOTE_PORT=9194


sed -i 's/Auth-Token/Authorization/g' /usr/bin/ip_tunnel.py
/usr/bin/tun4.py -g {{ grains['conductor']['root_datacenter'] }} && \
    exec /usr/bin/socat \
        TCP${TCPROXY_LISTEN_PROTO}-LISTEN:${TCPROXY_LISTEN_PORT},reuseaddr,fork \
        TCP${TCPROXY_REMOTE_PROTO}:${TCPROXY_REMOTE_ADDR}:${TCPROXY_REMOTE_PORT}
