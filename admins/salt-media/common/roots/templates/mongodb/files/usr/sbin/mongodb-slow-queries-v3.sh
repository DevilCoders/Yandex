#!/bin/bash

print_help() {
    echo "Usage: $0 [options]"
    echo
    echo -e "-h\tThis message."
    echo -e "-s\tShow slow queries."
    echo -e "-c\tShow collscans."
    echo -e "-p\tMongodb port."
    echo -e "-l\tMinimal number of slow queries which triggers critical condition."
    echo -e "-t\tTime in ms when a request is considered as slow."
    exit
}

#min number of slow queries
SQ_COUNT=1
#time period to monitor from now
PERIOD=300
#slow query in ms
TIME_TO_MONIT=300
#mongodb port
PORT=27018

while getopts "hscp:l:t:" opt; do
    case $opt in
        h) print_help;;
        c) MONITOR_COLLSCAN=1;;
        p) PORT=$OPTARG;;
        s) SHOW_ME_LOG=1;;
        l) SQ_COUNT=$OPTARG;;
        t) TIME_TO_MONIT=$OPTARG;;
    esac
done

# query mongo daemon
IS_MASTER=$(mongo --quiet --port=$PORT --eval "`cat /root/.mongorc.js` db.isMaster().ismaster" 2>/dev/null)
QUERY_COUNT=$(mongo --quiet --port=$PORT --eval "`cat /root/.mongorc.js` x = db.runCommand({serverStatus:1}).opcounters; print(x.query + x.insert + x.update + x.delete);" 2>/dev/null)

# process query counts
QUERY_TMP_FILE=/tmp/mongodb-slow-queries-v3.count
OLD_QUERY_COUNT=0
if [ -f $QUERY_TMP_FILE ]; then
    OLD_QUERY_COUNT=`cat $QUERY_TMP_FILE`;
fi
echo $QUERY_COUNT >$QUERY_TMP_FILE
QUERIES_TOTAL=$(($QUERY_COUNT - $OLD_QUERY_COUNT))
if [[ $SQ_COUNT =~ % ]]; then
    SQ_COUNT=${SQ_COUNT%?}
    SQ_COUNT=$(awk "BEGIN{print int($SQ_COUNT * $QUERIES_TOTAL / 100);}")
fi


if [[ "$IS_MASTER" == "false" ]]; then
  EXCLUDE_ON_SLAVE="aggregate"
fi

if [ "$SHOW_ME_LOG" ];then
  DUMPER="tee /dev/stderr"
else
  DUMPER="cat"
fi

declare -A SQ
eval SQ=(
$(timetail -t mongodb -n $PERIOD /var/log/mongodb/mongodb.log | \
    mawk -v time=$TIME_TO_MONIT -v show=$SHOW_ME_LOG '
/COLLSCAN/ {
    collscan++
    if (show) print $0 > "/dev/stderr"
}
/ms$/ &&\
!/('"$EXCLUDE_ON_SLAVE${EXCLUDE_ON_SLAVE:+|}"'moveChunk|command local.oplog.rs command: getMore)/ &&\
/COMMAND|QUERY/ {
    if(int($NF)>time){
        slow++
        if (show) print $0 > "/dev/stderr"
    }
}
END {
    printf("[slow]=%d [collscan]=%d\n", slow, collscan)
}')
)

if ((${SQ["slow"]} > SQ_COUNT)); then
    lvl=2; msg="${SQ["slow"]} slow queries of $QUERIES_TOTAL total per $((PERIOD / 60)) min"
fi

if test -n "$MONITOR_COLLSCAN" && ((${SQ["collscan"]:-0} > 0)); then
    lvl=2; msg="${SQ["collscan"]} COLLSCAN here${msg:+, }$msg"
fi

echo "${lvl:-0};${msg:-OK}"
