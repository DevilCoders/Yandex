#!/bin/bash

########## vars ##########
# threshold in ms for slow query
mon_thresh=200000
log_thresh=100000
log=/var/log/slowquery/slow_check.log
tmplast=/tmp/slowq.last.tmp
# 172800000ms = 2 days
# limit required to discard values like 18446744073709548ms which looks like some mysql init value or a mistake
mysql --defaults-file=/root/.my.cnf -Bse "select HOST,ID,md5(INFO),TIME_MS from information_schema.PROCESSLIST where COMMAND='Query' and TIME_MS>$log_thresh and TIME_MS<172800000;" > /tmp/slowq.tmp
maxtime=0

while read line
do
    data=`echo $line | egrep -o "(^[a-z0-9]+)\.([a-z]+)\.yandex\.net\:[0-9]+[[:space:]]([0-9]+)[[:space:]]([a-z0-9]+)[[:space:]]([0-9]+)"`
    time_ms=0
    if [[ $data != "" ]]
    then
        time_ms=$(echo $data | awk '{print $NF}')
        q_id=$(echo $data | awk '{print $(NF-2)}')
        q_hash=$(echo $data | awk '{print $(NF-1)}')
        # check for the same very long query in log and not write dublicate line into log about this query
        if [[ ! $(timetail -n $mon_thresh $log | grep $q_hash) ]]
        then
            if [[ $time_ms -gt $maxtime ]]
            then
                if [[ $time_ms -gt $log_thresh ]]
                then
                    echo "[$(date -R)] $data" >> $log
                    if [[ $time_ms -gt $mon_thresh ]]
                    then
                        maxtime=$time_ms
                    fi
                fi
            fi
        else
            maxtime=$time_ms
        fi
    fi
done < /tmp/slowq.tmp

if [[ $maxtime -ge $mon_thresh ]]
then
    echo "2;Detected query with time $maxtime ms, query id: $q_id"
else
    echo "0;OK"
fi

exit 0
