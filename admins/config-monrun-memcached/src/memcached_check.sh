#!/bin/bash

# default settings below

# systemctl bin
SYSTEMCTL=$(which systemctl)
# memcached pidfile
PIDFILE="/var/run/memcached.pid"
# file to save to the memcached stats
STATSFILE="/run/shm/memcached_stats"
# port on which listens memcached daemon
PORT=11211
# disable sending data to graphite(in common case we don't know project\group and ohers)
GRAPH=0
# prefix for graphite metric(by default empty, because metrics sending off)
GR_PREFIX=''
# critical threshold in % of total memcached memory
CRIT_THR=90
# warning threshold in % of total memcached memory
WARN_THR=80
# evictions thresholds, evictions per second
EVICT_WARN_THR=100
EVICT_CRIT_THR=200

# get configuration from file if it exists
if [[ -e /etc/memcached-check.conf ]]; then
    # config can include some or all settings
    # if setting present it config, it replaces dafault value
    . /etc/memcached-check.conf
fi

# ensure the memcached process is alive
if [[ -f $SYSTEMCTL ]]; then
    systemctl_status=$($SYSTEMCTL show -p ExecMainCode memcached | sed 's/ExecMainStatus=//g')
    if [[ $systemctl_status -ne 0 ]]; then
        echo "2;memcached process is dead"
        exit 0
    fi
else
    if [[ ! -f $PIDFILE ]]; then
        echo "2;memcached pid file not found"
        exit 0
    else
        PID=$(cat $PIDFILE)
        ps -p $PID 2>&1 1>/dev/null
        if [[ $? -ne 0 ]]; then
            echo "2;memcached process with $PID is dead"
        fi
    fi
fi

# save statistics
echo stats | nc -w 5 localhost $PORT > $STATSFILE
if [[ $? -ne 0 ]]
then
    echo "2;can't save mamcached stats to $STATSFILE"
    exit 0
fi

STATUS=0

BYTES=$(sed -n 's/STAT bytes \([0-9]*\)\r/\1/p' $STATSFILE)
LIMIT_MAXBYTES=$(sed -n 's/STAT limit_maxbytes \([0-9]*\)\r/\1/p' $STATSFILE)
EVICTS=$(sed -n 's/STAT evictions \([0-9]*\)\r/\1/p' $STATSFILE)
TS=$(sed -n 's/STAT time \([0-9]*\)\r/\1/p' $STATSFILE)

CRIT_THR_BYTES=$(($LIMIT_MAXBYTES * $CRIT_THR / 100))
WARN_THR_BYTES=$(($LIMIT_MAXBYTES * $WARN_THR / 100))

# send data to graphite
if [[ $GRAPH -eq 1 ]]; then
    GR_FQDN=$(hostname -f | sed "s/\./\_/g")
    BYTES_METRIC="${GR_PREFIX}.${GR_FQDN}.memcached.used_bytes ${BYTES} ${TS}"
    echo "$BYTES_METRIC" | nc -w 5 localhost 42000 2>&1 1>/dev/null
fi

DESCR="used $(($BYTES * 100 / $LIMIT_MAXBYTES))% of mem limit (warn ${WARN_THR}, crit ${CRIT_THR}); "
if [[ $BYTES -ge $CRIT_THR_BYTES ]]; then
    STATUS=2
elif [[ $BYTES -ge $WARN_THR_BYTES ]]; then
    STATUS=1
fi

if  [[ -e ${STATSFILE}.prev ]]; then
    EVICTS_PREV=$(sed -n 's/STAT evictions \([0-9]*\)\r/\1/p' ${STATSFILE}.prev)
    TS_PREV=$(sed -n 's/STAT time \([0-9]*\)\r/\1/p' ${STATSFILE}.prev)
    EVICTS_FREQ=$((($EVICTS - $EVICTS_PREV) / ($TS - $TS_PREV)))

    DESCR="${DESCR}${EVICTS_FREQ} evictions per second (warn ${EVICT_WARN_THR}, crit ${EVICT_CRIT_THR}); "
    if [[ $EVICTS_FREQ -gt $EVICT_CRIT_THR ]]; then
        STATUS=2
    elif [[ $EVICTS_FREQ -gt $EVICT_WARN_THR ]]; then
        if [[ $STATUS -lt 2 ]]; then
            STATUS=1
        fi
    fi
else
    DESCR="${DESCR}no evictions statistics yet"
fi

mv $STATSFILE ${STATSFILE}.prev

if [[ $? -ne 0 ]]; then
    STATUS=2
    DESCR="${DESCR}can't save mamcached prev stats to ${STATSFILE}.prev"
fi

echo "${STATUS};${DESCR}"
