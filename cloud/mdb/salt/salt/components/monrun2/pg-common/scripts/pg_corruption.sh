{% from "components/postgres/pg.jinja" import pg with context %}
#!/bin/bash

die () {
    echo "$1;$2"
    exit 0
}

while getopts "r:w:c:n:e:" OPTION
do
    case $OPTION in
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

watch_seconds=${WATCH_SECONDS:-600}
regex=${REGEX:-'([0-9]{4}-[0-9]{2}-[0-9]{2}\ [0-9]{2}\:[0-9]{2}\:[0-9]{2}.[0-9]{3}\ MSK)'}

logfile="{{ pg.log_file_path }}"

xx00_count=$(sudo -u postgres timetail -n "$watch_seconds" -j 100500000 -r "$regex" "$logfile" 2>&1 | fgrep 'ERROR' | grep -c -e 'XX00\(1\|2\)' -e "XX000.*right sibling's left-link doesn't match" -e "XX000.*btree level.*not found in index" -e "XX000.*unexpected chunk size" -e "XX000.*missing chunk number")

if [ "$xx00_count" -gt "0" ]
then
    die 2 "Possible data corruption: $xx00_count XX00 errors for last $watch_seconds seconds."
else
    die 0 OK
fi
