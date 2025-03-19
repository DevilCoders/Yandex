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

warn_limits=${WARN_LIMITS:-'100 100'}
crit_limits=${CRIT_LIMITS:-'1000 1000'}
watch_seconds=${WATCH_SECONDS:-600}
{% from "components/postgres/pg.jinja" import pg with context %}
{% if pg.connection_pooler == 'odyssey' %}
regex=${REGEX:-'^[A-Za-z]+\s([0-9]{2}\ [A-Za-z]{3}\ [0-9]{2}\:[0-9]{2}\:[0-9]{2}.[0-9]{3})'}
{% else %}
regex=${REGEX:-'([0-9]{4}-[0-9]{2}-[0-9]{2}\ [0-9]{2}\:[0-9]{2}\:[0-9]{2}.[0-9]{3})'}
{% endif %}

default_exclude='server conn crashed|query end, but query_start'
exclude=${EXCLUDE:-${default_exclude}}
if [ "$exclude" != "$default_exclude" ]
then
    exclude+="|${default_exclude}";
fi
read warn_warn_limit error_warn_limit <<<"$warn_limits"
read warn_crit_limit error_crit_limit <<<"$crit_limits"

{% set base='/var/log/postgresql/' %}
{% if pg.connection_pooler == 'odyssey' %}
logfiles="/var/log/odyssey/odyssey.log"
{% else %}
{% if salt['pillar.get']('data:pgbouncer:count', 1)==1 %}
logfiles="{{ base }}pgbouncer.log"
{% else %}
{% set l=[] %}
{% for c in range(salt['pillar.get']('data:pgbouncer:count')) %}
{% set i="%02d"|format(c|int) %}
{% do l.append(base + "pgbouncer" + ("%02d"|format(i|int)) + ".log") %}
{% endfor %}
logfiles="{{ l|join(' ') }}"
{% endif %}
{% endif %}

user="postgres"
{% if pg.connection_pooler == 'odyssey' %}
{% set error_keyword = 'error:' %}
{% set warning_keyword = 'warning:' %}
{% else %}
{% set error_keyword = 'ERROR' %}
{% set warning_keyword = 'WARNING' %}
{% endif %}

error_count=$(sudo -u "$user" timetail -n "$watch_seconds" -r "$regex" $logfiles 2>&1 | fgrep '{{ error_keyword }}'  | grep -Ev "$exclude" -c)
warn_count=$(sudo -u "$user" timetail -n "$watch_seconds" -r "$regex" $logfiles 2>&1 | fgrep '{{ warning_keyword }}' | grep -Ev "$exclude" -c)

if [ "$error_count" -gt "$error_crit_limit" ]
then
    die 2 "$error_count errors for last $watch_seconds seconds"
elif [ "$warn_count" -gt "$warn_crit_limit" ]
then
    die 2 "$warn_count warns for last $watch_seconds seconds"
elif [ "$error_count" -gt "$error_warn_limit" ]
then
    die 1 "$error_count errors for last $watch_seconds seconds"
elif [ "$warn_count" -gt "$warn_warn_limit" ]
then
    die 1 "$warn_count warns for last $watch_seconds seconds"
else
    die 0 OK
fi
