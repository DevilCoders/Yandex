#!/bin/bash

die () {
    echo "$1;$2"
    exit 0
}

while getopts "r:f:w:c:n:" OPTION
do
    case $OPTION in
        w)
            WARN_LIMITS="$OPTARG"
        ;;
        c)
            CRIT_LIMITS="$OPTARG"
        ;;
        n)
            WATCH_SECONDS="$OPTARG"
        ;;
        f)
            LOG_DIR="$OPTARG"
        ;;
        r)
            REGEX="$OPTARG"
        ;;
    esac
done

log_dir=${LOG_DIR:-/var/lib/pgsql/9.4/data/pg_log}
warn_limits=${WARN_LIMITS:-'1 20'}
crit_limits=${CRIT_LIMITS:-'10000 10000'}
watch_seconds=${WATCH_SECONDS:-600}
regex=${REGEX:-'([0-9]{4}-[0-9]{2}-[0-9]{2}\ [0-9]{2}\:[0-9]{2}\:[0-9]{2}.[0-9]{3}\ MSK)'}

read fatal_warn_limit error_warn_limit <<<"$warn_limits"
read fatal_crit_limit error_crit_limit <<<"$crit_limits"

logfile=$(sudo -u postgres ls "$log_dir" | grep '.log' | sort | tail -n1)

error_count=$(sudo -u postgres timetail -n "$watch_seconds" -r "$regex" "$log_dir/$logfile" 2>&1 | grep -v 'in a read-only transaction' | grep -E 'ERROR:' -c)
fatal_count=$(sudo -u postgres timetail -n "$watch_seconds" -r "$regex" "$log_dir/$logfile" 2>&1 | grep -E 'FATAL:' -c)

if [ "$fatal_count" -gt "$fatal_crit_limit" ]
then
    die 2 "$fatal_count fatals for last $watch_seconds seconds"
elif [ "$error_count" -gt "$error_crit_limit" ]
then
    die 2 "$error_count errors for last $watch_seconds seconds"
elif [ "$fatal_count" -gt "$fatal_warn_limit" ]
then
    die 1 "$fatal_count fatals for last $watch_seconds seconds"
elif [ "$error_count" -gt "$error_warn_limit" ]
then
    die 1 "$error_count errors for last $watch_seconds seconds"
else
    die 0 OK
fi
