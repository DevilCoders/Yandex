#!/bin/bash

unset runlevel
unset RUNLEVEL

if [ -z $1 ]; then
    echo "$(basename $0) [N], where N - min. ok runlevel number"
    exit 0
fi
OK_RUNLEVEL=$1
CUR_RUNLEVEL=$(/sbin/runlevel 2>/dev/null| cut -f2 -d\ )

if [ $CUR_RUNLEVEL -ge $OK_RUNLEVEL ] && [ $CUR_RUNLEVEL -lt 6 ];then
    echo "0;ok"
else
    echo "2;Runlevel is $(/sbin/runlevel)"
fi
