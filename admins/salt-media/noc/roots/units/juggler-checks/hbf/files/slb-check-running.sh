#!/bin/bash
echo "PASSIVE-CHECK:slb-check_running;`/usr/sbin/daemon_check.sh -E 'bin/sh /usr/sbin/hbf-check.sh .+ /home/hbf/slb-check'`"

