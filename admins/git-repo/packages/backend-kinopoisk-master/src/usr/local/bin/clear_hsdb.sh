#!/usr/bin/env bash

usage(){
    echo "Usage: $0 DB DB_IP ACTION (disable | enable) DB_TO"
}

ARGS=`getopt -o h --long -- "$@"`
eval set -- "$ARGS"

while true ; do
    case "$1" in
        --) shift ; break ;;
        *) echo "Internal error!"; usage; exit 1;;
    esac
done

if [ "${#@}" -lt 4 ];then
    usage
    exit 1
fi

DB=$1
DB_IP=$2

ACTION=$3
DB_TO=$4

while ! mkdir /tmp/$1.lock &>/dev/null ; do sleep 1h ; done
trap 'rm -r /tmp/$1.lock' EXIT

curl --connect-timeout 10 -s -o /var/tmp/$DB http://c.yandex-team.ru/api-cached/groups2hosts/kp-backend

if [ $? -ne 7 ]; then

HOSTS_FILE="/etc/hosts"
BK=`cat /var/tmp/$DB`
HOSTS=$BK" master01h.kp.yandex.net conv01h.kp.yandex.net"
COUNTER=1

for host in $HOSTS; do
    #echo $host
    ROW=`ssh -q ${host} "cat $HOSTS_FILE | grep $DB"`
    if [ "$ROW" ]; then
        if [ $ACTION = "disable" ]; then
            #echo "Already disabled: $ROW"
        else
            #echo "Enabling"
            ssh -q ${host} "sed -i s/$DB_TO\ $DB// $HOSTS_FILE"
            COUNTER=$((COUNTER+1))
            if [[ $COUNTER -gt 10 ]]; then
                #echo "Sleeping 20 min to fill cache"
                sleep 1200
                COUNTER=1
            fi
        fi
    else
        if [ $ACTION = "disable" ]; then
            #echo "Disabling"
            ssh -q ${host} "echo \"$DB_TO $DB\" >> $HOSTS_FILE"
        else
            #echo "Already enabled"
        fi
    fi
done

# wait for connections go away
if [ $ACTION = "disable" ]; then
    while : ; do
        CONNS=`ssh -q $DB_IP "mysql -e 'show processlist'|grep handlersocket|awk -F\":\" '{print \\$2}' | awk '{print \\$2}' | awk '{ sum += \\$1; } END { print sum; }'  "`
        #echo $CONNS
        [[ $CONNS -gt 0 ]] || break
        sleep 10
    done
    # no connections here
    ssh -q $DB_IP "mysql cache < /root/truncate.sql"
    #echo "done"
fi

else
        #echo "can't connect to conductor";
        exit 0
fi
