#!/bin/bash

PASSWORD="sULucdg65pQk"
RSYNC_USER="disk"


HOST=$1
RSYNC_PATH=$2
TARGET_DIR=$(readlink -f $3)
TARGET_SIZE=$4

watcher() {
    PID=$1
    SIZE=$2

    while true ; do 
        size1=$(du -ms . | cut -f1)
        sleep 120
        size2=$(du -ms . | cut -f1)

        if [ $[size2-size1] -lt 3 ] ; then
            echo "Watcher: size diff is $[size2-size1], killing $PID and exiting"
            kill -9 $PID
            sleep 10
            echo "Watcer: killing any other rsync processes":
            if [ -f /var/run/rsync.pid ]; then
                /usr/bin/pgrep rsync | grep -v -f /var/run/rsync.pid | xargs -I {} kill -9 {}
                exit 0
            else
                /usr/bin/pgrep rsync | xargs -I {} kill -9 {}
                exit 0
            fi
        fi
        echo "int(${size2}/${SIZE}*100)" | perl -ne 'print eval' 
        echo
    done
}

usage() {
    echo "USAGE: $0 HOST TARGET_DIR TARGET_SIZE"
    exit 0
}

[ -z $RSYNC_PATH ] && usage
[ -z $TARGET_SIZE ] && usage
[ -z $TARGET_DIR ] && usage
[ $TARGET_SIZE -le 0 ] && usage

if [ ! -d $TARGET_DIR ] ; then
    echo "Cannot find dir $TARGET_DIR"
    exit 1
fi

cd $TARGET_DIR
while true; do
    RSYNC_PASSWORD=$PASSWORD rsync -aH --delete rsync://${RSYNC_USER}@${HOST}/${RSYNC_PATH}/ . & 
    WATCHED_PID=$!
    echo "Started rsync with pid $WATCHED_PID"
    sleep 10                #give rsync some time to catch up
    watcher $WATCHED_PID $TARGET_SIZE &
    WATCHER_PID=$!
    echo "Started guard process with pid $WATCHER_PID"
    wait $WATCHED_PID

    if [ $? == 0 ]; then
        break
    else
        echo "rsync exited with non-zero code, trying again"
        echo "Killing guard process $WATCHER_PID"
        kill -9 $WATCHER_PID
        sleep 5
    fi
done

