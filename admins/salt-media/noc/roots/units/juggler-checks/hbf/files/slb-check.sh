#!/bin/sh

SERVER="$1"

URL="http://$SERVER/slb-check"
RESOLVE="$SERVER:80:127.0.0.1"
SERVICE="slb-check-$SERVER"

RES=$(curl -m 10 -o /dev/null -s -w "%{http_code}" --resolve "$RESOLVE" "$URL")

curl_status=$?
if [ -z "$RES" ]; then
    echo "PASSIVE-CHECK:$SERVICE;2;curl error $curl_status"
elif [ "$RES" -lt "200" -o "$RES" -ge "400" ]; then
    echo "PASSIVE-CHECK:$SERVICE;2;$RES"
else
    echo "PASSIVE-CHECK:$SERVICE;0;OK"
fi
