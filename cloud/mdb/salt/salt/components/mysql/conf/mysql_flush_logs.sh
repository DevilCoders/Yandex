#!/bin/bash

if ! pidof mysqld > /dev/null
then
    exit 0
fi

declare -A LOGS=(
    ["error-log"]="FLUSH LOCAL ERROR LOGS"
    ["query-log"]="FLUSH LOCAL GENERAL LOGS"
    ["slow-log"]="FLUSH LOCAL SLOW LOGS"
    ["audit-log"]="SET GLOBAL audit_log_flush = 1"
)

for log in ${!LOGS[@]}
do
    if [ "$1" = "$log" ] || [ -f /var/run/mysql-flush-$log -a -z "$1" ]
    then
        mysql -e "${LOGS[$log]}" \
            && rm -f /var/run/mysql-flush-$log \
            || touch /var/run/mysql-flush-$log
    fi
done
