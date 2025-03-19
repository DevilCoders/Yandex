#!/bin/bash

set -u

function show_usage(){
    printf "%s WARN CRIT FILENAME\n" `basename $0`
}


time_from_seconds() {
    d=`expr $1 / 86400`
    h=`expr $1 % 86400 / 3600`
    m=`expr $1 % 3600 / 60`
    s=`expr $1 % 60`
    printf "%d days %02d hours %02d minutes %02d seconds\n" $d $h $m $s
}

if [ $# -ne 3 ] ; then
    show_usage
    exit 1
fi

WARN=$1
CRIT=$2
TFILE=$3

CODE=2
MSG="Default message. Check script."

ps auxwww| grep -vP '(daemon|grep)' | grep -q safe-rsync
RSYNC_RUNNING=$?

if [ ! -e ${TFILE} ] ; then
    if [ "${RSYNC_RUNNING}" -eq "0" ] ; then
        MSG="Failed, refill from replica: flag-file does not exist, but safe-rsync proccess was found"
        CODE=2
    else
        MSG="OK, refill from replica: inactive (flag-file does not exist)"
        CODE=0
    fi

else 
    if [ "${RSYNC_RUNNING}" -eq "1" ] ; then
        MSG="Failed, refill from replica: flag-file exists, but no safe-rsync proccess was found"
        CODE=2
    else
        CURRENT_TS=`date "+%s"`
        TFILE_TS=`stat --format "%Z" ${TFILE}`

        DELTA_TS=`expr "${CURRENT_TS}" - "${TFILE_TS}"`
        DELTA_TIME=`time_from_seconds ${DELTA_TS}`

        if [ "${DELTA_TS}" -ge "${CRIT}" ] ; then
            MSG="Failed, refill was started ${DELTA_TIME} ago"
            CODE=2
        elif [ "${DELTA_TS}" -gt "${WARN}" ] ; then
            MSG="OK, refill was started ${DELTA_TIME} ago"
            CODE=1
        else 
            CODE=0
        fi
    fi
fi


printf "%d; %s\n" "${CODE}" "${MSG}"

exit 0


