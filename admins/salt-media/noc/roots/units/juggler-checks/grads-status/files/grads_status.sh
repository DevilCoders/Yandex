#!/usr/bin/env bash

echo "PASSIVE-CHECK:grad-server_status;`/usr/sbin/daemon_check.sh '/opt/grad/pyvenv/bin/python3.. /opt/grad/grad_server.py -c /etc/grad/grad_server.yml --config-directory /etc/grad/conf.d \[main server\]'`"

