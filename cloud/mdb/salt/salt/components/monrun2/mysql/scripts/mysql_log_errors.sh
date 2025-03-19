#!/bin/bash

die () {
    echo "$1;$2"
    exit 0
}

while getopts "r:w:c:n:e:" OPTION
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
        r)
            REGEX="$OPTARG"
        ;;
        e)
            EXCLUDE="$OPTARG"
        ;;
    esac
done

warn_limits=${WARN_LIMITS:-'100'}
crit_limits=${CRIT_LIMITS:-'1000'}
watch_seconds=${WATCH_SECONDS:-600}
regex=${REGEX:-'([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}\:[0-9]{2}\:[0-9]{2}.[0-9]{6}\+[0-9]{2}\:[0-9]{2})'}
exclude=${EXCLUDE:-'^$'}
read error_warn_limit <<<"$warn_limits"
read error_crit_limit <<<"$crit_limits"

logfile="/var/log/mysql/error.log"

error_count=$(timetail -n "$watch_seconds" -j 100500000 -r "$regex" "$logfile" 2>&1 | grep -v -e "$exclude" | fgrep -c 'ERROR')

if [ "$error_count" -gt "$error_crit_limit" ]
then
    die 2 "$error_count errors for last $watch_seconds seconds"
elif [ "$error_count" -gt "$error_warn_limit" ]
then
    die 1 "$error_count errors for last $watch_seconds seconds"
else
    die 0 OK
fi
