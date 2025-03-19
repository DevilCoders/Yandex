#!/usr/bin/env bash


STATE_FILE="/var/cache/mongo-restore/last_state"
if [ ! -f "$STATE_FILE" ]; then
    echo "2; There is no saved last mongo restore state (no file): $STATE_FILE"
    exit 0
fi
STATE_FILE_MTIME=$(stat -c %Y $STATE_FILE 2>/dev/null)
STATE_FILE_DATA=$(cat $STATE_FILE)

CRIT_DELTA=${1:-129600}
CURTIME=$(date +%s)
CUR_DELTA=$(($CURTIME-$STATE_FILE_MTIME))

if [ $CUR_DELTA -gt $CRIT_DELTA ]; then
    echo "2; Last mongo restore state too old: $STATE_FILE (last mtime change $CUR_DELTA secs ago)"
    exit 0
fi

if [ $STATE_FILE_DATA -ne 0 ]; then
    echo "2; Last mongo restore ended with not null code: $STATE_FILE_DATA"
    exit 0
fi

echo "0;Ok"
