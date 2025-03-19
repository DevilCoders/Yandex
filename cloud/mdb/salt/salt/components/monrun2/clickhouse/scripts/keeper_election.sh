#!/bin/sh

die () {
    echo "$1;$2"
    exit 0
}

PERIOD=600
OCCURENCES_CRIT=8
FILE=/var/log/clickhouse-keeper/clickhouse-keeper.log

test -f ${FILE} || die 0 OK

elections=$(timetail -t java -n${PERIOD} /var/log/clickhouse-keeper/clickhouse-keeper.log | fgrep -c 'initiate leader election')

if [ ${elections} -ge ${OCCURENCES_CRIT} ]
then
    die 2 "${elections} reelections for last ${PERIOD} sec"
fi

die 0 OK
