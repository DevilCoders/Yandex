{% from "components/postgres/pg.jinja" import pg with context %}
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

warn_limits=${WARN_LIMITS:-'20 100'}
crit_limits=${CRIT_LIMITS:-'1000 10000'}
watch_seconds=${WATCH_SECONDS:-600}
regex=${REGEX:-'([0-9]{4}-[0-9]{2}-[0-9]{2}\ [0-9]{2}\:[0-9]{2}\:[0-9]{2}.[0-9]{3}\ MSK)'}
{% if not salt['pillar.get']('data:monrun:pg_log_errors:ignore_read_only', True) %}
exclude=${EXCLUDE:-'^$'}
{% else %}
exclude=${EXCLUDE:-'in a read-only transaction'}
if [ "$exclude" != 'in a read-only transaction' ]
then 
    exclude+='\|in a read-only transaction';
fi
{% endif %}
{% if salt['pillar.get']('data:monrun:pg_log_errors:exclude_on_replicas', False) %}
is_replica=$(/usr/bin/psql 'user=monitor dbname=postgres' -AXqtc 'select pg_is_in_recovery()' 2>/dev/null)
if [ "$is_replica" = 't' ]
then
    exclude+={{ salt['pillar.get']('data:monrun:pg_log_errors:exclude_on_replicas') }};
fi
{% endif %}
read fatal_warn_limit error_warn_limit <<<"$warn_limits"
read fatal_crit_limit error_crit_limit <<<"$crit_limits"

logfile="{{ pg.log_file_path }}"

error_count=$(sudo -u postgres timetail -n "$watch_seconds" -j 100500000 -r "$regex" "$logfile" 2>&1 | grep -v -e "$exclude" | fgrep -c 'ERROR')
fatal_count=$(sudo -u postgres timetail -n "$watch_seconds" -j 100500000 -r "$regex" "$logfile" 2>&1 | grep -v -e "$exclude" | fgrep -c 'FATAL')

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
