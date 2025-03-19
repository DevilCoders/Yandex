#!/bin/bash

me=${0##*/} # strip path
me=${me%.*} # strip extension

die () {
    echo "$1;$2"
    exit 0
}

ABSOLUTE=0

while getopts "c:w:a" OPTION
do
    case $OPTION in
        c)
            CRIT_LIMIT="$OPTARG"
        ;;
        w)
            WARN_LIMIT="$OPTARG"
        ;;
        a)
            ABSOLUTE=1
        ;;
    esac
done

case $ABSOLUTE in
    1)
        crit_limit=${CRIT_LIMIT:-300}
        warn_limit=${WARN_LIMIT:-50}
    ;;
    0)
        cpu=$(fgrep -c processor /proc/cpuinfo)
        crit_limit=${CRIT_LIMIT:-0.3}
        warn_limit=${WARN_LIMIT:-0.6}
        crit_limit=$(echo "$crit_limit*$cpu" | bc)
        warn_limit=$(echo "$warn_limit*$cpu" | bc)
    ;;
esac

la5=$(awk '{print $2}' /proc/loadavg | cut -f1 -d\.)

if [ "$ABSOLUTE" = "1" ]
then
    if [ "$la5" -gt "$crit_limit" ]
    then
        die 2 "$la5"
    elif [ "$la5" -gt "$warn_limit" ]
    then
        die 1 "$la5"
    else
        die 0 "OK"
    fi
else
    cmp_crit=$(echo "$la5 > $crit_limit" | bc)
    cmp_warn=$(echo "$la5 > $warn_limit" | bc)
    if [ "$cmp_crit" = "1" ]
    then
        die 2 "$la5"
    elif [ "$cmp_warn" = "1" ]
    then
        die 1 "$la5"
    else
        die 0 "OK"
    fi
fi
