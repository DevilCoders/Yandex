#!/bin/bash
#set -x
RSYNC_SERVER="vdb.yandex.ru"
RSYNC_SHARE="infection_info"
DST_PATH="/var/lib/yandex/yandex-safebrowsing-data/data"
RAND=$(echo $RANDOM | cut -c -2)

if [ `cat /etc/yandex/environment.type` == "development" ]; then
	RSYNC_SERVER="vdb-dev.yandex.ru"
fi

sleep $RAND

if [ -d ${DST_PATH} ]; then
	for i in `seq 1 5`; do
		rsync -a --contimeout 30  ${RSYNC_SERVER}::${RSYNC_SHARE}/*.xml ${DST_PATH} 1>/dev/null 2>&1
		if [ $? -eq 0 ]; then
			exit 0
		else
			echo "Connection timeout to ${RSYNC_SERVER}::${RSYNC_SHARE} retry ${1}/5" 1>&2
			sleep 2
		fi
	done
	exit 1 
else
	echo "Please create ${DST_PATH} dir!"
	exit 1
fi

