#!/bin/bash

set -e

LOGFILE="/var/log/nginx/access.log"

STALE_COUNT=`timetail -n 300 $LOGFILE | grep weather-api | grep -c STALE || true`
OVERALL=`timetail -n 300 $LOGFILE | grep -c weather-api`

STALE_PERCENT=`echo "scale=0;$STALE_COUNT*100/$OVERALL" | bc -l`

if [ -z "$STALE_PERCENT" ];
then
	echo "0;OK";
else
	if [ $STALE_PERCENT -gt 0 ];
	then
		echo "2; too many STALE answers from cache - $STALE_PERCENT";
	else
		echo "0;OK";
	fi
fi
