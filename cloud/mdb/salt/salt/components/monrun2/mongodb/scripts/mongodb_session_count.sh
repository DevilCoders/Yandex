#!/bin/bash

die () {
    echo "$1;$2"
    exit 0
}

FILE=/var/log/mongodb/mdb-sessions-watchdog.log

if [ -f "${FILE}" ]
then
    count=$(grep -c ERROR "${FILE}")
    if [ "${count}" -gt 0 ]
    then
        die 1 "Sessions count watchdog errors in ${FILE}: ${count}"
    fi
fi

die 0 OK
