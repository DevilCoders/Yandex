#!/bin/sh

for file in `find /var/log/mirrors/ -name "*.log"`; do
    NAME="$(echo $file | awk -F/ '{ print $NF }' | awk -F_ '{ print $1 }')"
    if [ ! -f "/var/lock/$NAME-mirror" ]; then
        rm -f $file
    fi
done

