#!/bin/sh -e

outstanding=`echo stats | nc -i1 localhost 2181 2>/dev/null | egrep Outstanding | awk '{print $2}'`

if [ "$outstanding" -eq "$outstanding" ] 2>/dev/null; then
    if [ "$outstanding" -ge 100 ]; then
        exit 1
    fi
    exit 0
else
    exit 1
fi

