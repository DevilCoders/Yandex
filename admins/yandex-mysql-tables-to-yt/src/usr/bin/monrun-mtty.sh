#!/bin/bash

FILE="/tmp/mysql-tables-to-yt.log"


RES="0"
if [ -f "$FILE" ]
then
    LASTMOD=$(stat -c %Y $FILE)
    NOW=$(date +%s)
    if (( ${NOW}-3600*30 > ${LASTMOD} ))
    then
        RES="2"
    else
        RES=$(cat "$FILE")
    fi
fi
if [ "$RES" != "0" ]
then
    echo "2;mysql-tables-to-yt error"
else
    echo "0;OK"
fi
