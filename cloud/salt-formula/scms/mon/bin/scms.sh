#!/bin/bash

name=scms
check=$(exec 3>&1; /usr/bin/yc-scms --config=/etc/yc/scms/config.toml --status 2>&1 >&3 | grep -v GRPC_LINUX_EPOLL >&2)

if [[ "$check" ]] ; then
    echo "PASSIVE-CHECK:$check"
else
    echo "PASSIVE-CHECK:$name;2;Down"
fi
