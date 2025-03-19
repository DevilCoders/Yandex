#!/bin/bash

LOG=/var/log/mysql/mysqld_watchdog.log
MYADMIN="/usr/bin/mysqladmin"
# remember ooms and fals for peroid in seconds
PERIOD=$((3600 * 24))
# count any mysql fails not only happened with oom
COUNT_ANY=true
# where to save states(facts of restart)
STATFILE="/var/cache/mysqld_watchdog/states"
# thresholds
WARN_THR=1
CRIT_THR=2

if [[ -f /etc/monitoring/mysqld_watchdog.conf ]]
then
    source /etc/monitoring/mysqld_watchdog.conf
fi

function log()
{
    echo $(date)" $1" >> $LOG
}

function isexists()
{
    data=$1
    while read line
    do
        if [[ "$data" == "$line" ]]
        then
            return 0
        fi
    done < $STATFILE
    return 1
}

function is_refill_running()
{
    MYSQL_REFILL_FILE="/tmp/mysql_refill.tmp"
    if [ -f "$MYSQL_REFILL_FILE" ]
    then
        if [ $(( $(date +%s) - $(stat --format "%Y" "$MYSQL_REFILL_FILE") > (60*120) )) -eq 1 ]
        then
            echo "2;Refill is running more than 2 hours"
            exit 0
        else
            echo "1;Refill is running"
            exit 0
        fi
    fi

    if  pgrep -f "\btar\b"          >> /dev/null ;
        pgrep -f "\bnc\b"           >> /dev/null ||
        pgrep -f "\binnobackupex\b" >> /dev/null ;  then
        return 0
    else
        return 1
    fi
}

function checkoom()
{
    dt=$(dmesg -T | grep "oom_reaper:" | grep mysql | tail -1 | grep -Po "^\[[\S+\s]*\]" | tr -d "[]")
    isexists "$dt"
    exists=$?
    if [[ "$dt" != "" ]] && [[ $exists -eq 1 ]]
    then

        log "OOM detected!"
        echo "$dt" >> $STATFILE
        return 0
    else
        return 1
    fi
}

function flush_stats()
{
    now=$(date +%s)
    while read line
    do
        echo $line | grep -P "[\S\s]+" > /dev/null
        if [[ $? -eq 0 ]]
        then
            ts=$(date -d "$line" +%s)
            timediff=$(($now - $ts))
            if [[ $timediff -gt $PERIOD ]]
            then
                log "Removed old value $line from stats file."
                sed -i "/$line/d" $STATFILE
            fi
        fi
    done < $STATFILE
}

function monitor()
{
    if [[ ! -f $STATFILE ]]
    then
        echo "0;State file $STATFILE does not exists."
        return 0
    fi
    flush_stats
    oom_num=$(wc -l $STATFILE | awk '{print $1}')
    if [[ $oom_num -ge $CRIT_THR ]]
    then
        echo "2;CRIT: MySQL autorestared $oom_num times for $PERIOD sec"
        return 0
    elif [[ $oom_num -ge $WARN_THR ]]
    then
        echo "1;WARN: MySQL autorestarted $oom_num times for $PERIOD sec"
        return 0
    else
        echo "0;OK"
        return 0
    fi
}

function restartmysql()
{
    log "mysql is down; do autorestart..."
    /etc/init.d/mysql restart
    log "autorestart done."
}

function fixmysql()
{
    if [[ ! -f $MYADMIN ]]
    then
        log "$MYADMIN not found."
        return 0
    fi
    $MYADMIN --defaults-file=/etc/mysql/client.cnf ping 2>&1 1> /dev/null
    status=$?
    isoom=$(checkoom)
    if [[ $status -ne 0 ]] || [[ $isoom -eq 1 ]]
    then
        if [[ $COUNT_ANY ]]
        then
            date >> $STATFILE
        fi
        restartmysql
    fi
}

function main()
{
    statdir=$(dirname $STATFILE)
    if [[ ! -d $statdir ]]
    then
        mkdir -p $statdir
    fi
    is_refill_running && echo "0;MySQL refill in progress." && return 0
    if [[ "$1" != "" ]]
    then
        if [[ "$1" == "fix" ]]
        then
            fixmysql
        elif [[ "$1" == "check" ]]
        then
            monitor
        else
            echo "1; $0 unknown argument."
            exit 0
        fi
    else
        # only check if no args
        monitor
    fi
}

main $1
