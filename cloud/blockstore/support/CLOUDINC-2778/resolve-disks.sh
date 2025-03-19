#!/usr/bin/env bash

while read DISK_ID; do

    TABLET_ID=`curl -s "http://localhost:8766/blockstore/service?Volume=$DISK_ID&action=search" | grep -Eo "TabletID=([0-9]+)" | grep -Eo "[0-9]+"`

    if [ -n "$TABLET_ID" ]
    then
        ERR_MSG=`curl -s "http://localhost:8766/tablets/app?TabletID=$TABLET_ID" | grep "Tablet pipe with $TABLET_ID is not connected with status: ERROR"`
        if [ -z "$ERR_MSG" ]
        then
            echo "$DISK_ID $TABLET_ID OK"
        else
            echo "$DISK_ID $TABLET_ID BROKEN"
        fi
    else
        echo "$DISK_ID NOT FOUND"
    fi

done
