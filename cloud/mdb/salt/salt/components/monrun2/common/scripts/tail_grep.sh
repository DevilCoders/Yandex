#!/bin/bash

die () {
    echo "$1;$2"
    exit 0
}

DEFAULT_LOG_FILE=/var/log/syslog

grep_exclude_file="/etc/monitoring/grep_excludes"

while getopts "g:w:c:s:l:r:o:x:" OPTION
do
    case $OPTION in
        w)
            WARN_LIMIT="$OPTARG"
        ;;
        g)
            GREP_STRING="$OPTARG"
        ;;
        c)
            CRIT_LIMIT="$OPTARG"
        ;;
        s)
            WATCH_SECONDS="$OPTARG"
        ;;
        l)
            LOG_FILE="$OPTARG"
        ;;
        r)
            REGEX="$OPTARG"
        ;;
        o)
            GREP_OPTS="$OPTARG"
        ;;
        x)
            grep_exclude_file="$OPTARG"
        ;;
    esac
done

warn_limit=${WARN_LIMIT:-3}
crit_limit=${CRIT_LIMIT:-10}
watch_seconds=${WATCH_SECONDS:-600}
log_file=${LOG_FILE:-$DEFAULT_LOG_FILE}
grep_string=${GREP_STRING:-restarted}
regex=${REGEX:-}
grep_opts=${GREP_OPTS:-"-F"}

if ! which timetail >/dev/null 2>&1
then
    die 1 "timetail not found"
fi

if [ -f "${log_file}" ]; then
    if [ "$regex" = "" ]
    then
        regex_opt="-t syslog"
    else
        regex_opt="-r \"$regex\""
    fi
    if [ -f "$grep_exclude_file" ]; then
        count=$(timetail $regex_opt -n "$watch_seconds" "$log_file" 2>&1 | grep $grep_opts "$grep_string" | grep -vf $grep_exclude_file -c)
    else
        count=$(timetail $regex_opt -n "$watch_seconds" "$log_file" 2>&1 | grep $grep_opts -c "$grep_string")
    fi   

    if [ "$count" -gt "$crit_limit" ]; then
        die 2 "$count $grep_string for last $watch_seconds seconds"
    elif [ "$count" -gt "$warn_limit" ]; then
        die 1 "$count $grep_string for last $watch_seconds seconds"
    else
        die 0 "OK"
    fi
else
    die 1 "File $log_file not found"
fi
