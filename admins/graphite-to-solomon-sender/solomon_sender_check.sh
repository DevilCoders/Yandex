#!/bin/bash

STATUS_FILE="/tmp/graphite-to-solomon-sender/healthcheck.txt"
FILE_TTL_CRIT=300

# CADMIN-7536: check modification time
now=$(date +%s)
lastmodify=$(stat -c %Y $STATUS_FILE)
timedelta=$(($now - $lastmodify))

if [[ $timedelta -ge $FILE_TTL_CRIT ]]
then
	echo "2;${STATUS_FILE} not modified last ${timedelta}sec."
    exit 0
fi

cat $STATUS_FILE | grep -P "^\d" || echo "1;Warn:writing status."
