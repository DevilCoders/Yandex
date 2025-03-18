#!/bin/bash

[ -f /etc/redhat-release ] && DISTR='RHEL' || DISTR='Ubuntu'
[ -f /etc/issue ] && DISTVER=$(cat /etc/issue | /usr/bin/tr -s ' ' | /usr/bin/cut -d ' ' -f 2) || DISTVER='UNKNOWN'

SCAN=""
if [[ "$@" =~ "-i" ]] || [[ "$@" =~ "--iface" ]]; then
    true
elif [[ $DISTVER =~ ^[2-9][0-9].[0-9][0-9].?[0-9]?$ ]] && [[ $DISTR = 'Ubuntu' ]]; then
    SCAN="--scan"
fi

/usr/lib/yandex/python-netconfig/generate_interfaces.py "$SCAN" "$@"
