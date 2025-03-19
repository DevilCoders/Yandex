#!/bin/bash

set -u


if [ $# -lt 1 ]; then
    echo "$0 SERVICE [MAX_RESTARTS] [CHECK_SECONDS]"
    exit 2
fi

service=$1
max_restarts=${2:-3}
check_seconds=${3:-600}

log_start_string="wd-${service}: seems dead, executing: service $service start"
stopfile="/var/run/wd-${service}.stop"
logfile="/var/log/syslog"


service_pid=$( service ${service} status | grep -oP 'running, process \K\d+' )
if [ -n "$service_pid" ]; then
    if kill -0 $service_pid 2>/dev/null; then
        # pid exists. exit
        exit 0
    fi
fi

if [ -f $stopfile ]; then
    exit 0
fi

restarts_count=$( timetail -t syslog -n $check_seconds $logfile | grep -c "$log_start_string" )

if [ $restarts_count -ge $max_restarts ]; then
    logger -p daemon.err "wd-${service}: seems dead, but will not run due to restarts count: \
$restarts_count during $check_seconds seconds."
    exit 1
fi


logger -p daemon.err "$log_start_string"
service $service start

