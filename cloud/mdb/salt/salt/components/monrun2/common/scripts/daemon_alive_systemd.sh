#!/bin/bash

die () {
    printf "%s;%s\n" "$1" "$2"
    exit 0
}

daemon="$1"

is_enabled=$(/bin/systemctl is-enabled "${daemon}" 2>/dev/null)
if [ "enabled" != "$is_enabled" ]
then
    die 0 "${daemon} unit not found or disabled"
fi

is_active=$(/bin/systemctl is-active "${daemon}") 
if [ "active" = "$is_active" ]
then
    die 0 "OK"
fi

status=$(/bin/systemctl status "${daemon}" 2>/dev/null | egrep -o 'Active: (.*)' | cut -d';' -f 1)
die 2 "${status}"
