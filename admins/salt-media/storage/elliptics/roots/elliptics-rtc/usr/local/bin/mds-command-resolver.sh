#!/bin/bash

if [[ $1 == "action" ]]; then
    (
    flock -w 1 -n 9 || exit 2
    if [[ $2 == "shutdown" ]]; then
        unlink /etc/service/elliptics
        sleep 1
        sync
        echo "Elliptics shutdowned."
        exit 0
    elif [[ $2 == "up" ]]; then
        if [ ! -h /etc/service/elliptics ]; then
            ln -s /etc/sv/elliptics /etc/service/
        fi
    fi
    ) 9>/var/lock/mds-ops-action.lock
fi

