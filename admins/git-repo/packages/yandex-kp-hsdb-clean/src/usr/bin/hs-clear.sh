#!/bin/sh

DU=/usr/bin/du
IPT=/sbin/iptables
MAX_SLEEP=300
CLEAN_THRESHOLD=150
ZK_FLOCK="/usr/bin/zk-flock"
ZK_KEY="HS_CLEAR"

function downtime_host
{
    OK=0
    USE_DOWNTIME=1
    if [[ $USE_DOWNTIME -ne 0 ]]; then
        for host in $(curl -ks "http://c.yandex-team.ru/api/groups2hosts/media-jmon"); do 
	    DT_HOURS=3
	    DT_DESCRIPTION="HS clean"	
            echo  "Trying to downtime for $DT_HOURS hours at $(hostname)"
            curl -ks "http://${host}:8998/api/downtimes/set_downtime?do=1&object_name=$(hostname)&end_time=%2B${DT_HOURS}hours&description=${DT_DESCRIPTION}"
            curl -ks "http://${host}:8998/api/downtimes/set_downtime?do=1&object_name=kp-front&service_name=500-http-nginx-rating&end_time=%2B${DT_HOURS}hours&description=${DT_DESCRIPTION}"
            curl -ks "http://${host}:8998/api/downtimes/set_downtime?do=1&object_name=kp-backend&service_name=haproxy&end_time=%2B${DT_HOURS}hours&description=${DT_DESCRIPTION}"
            OK=$?
            if [[ ${OK} == 0 ]]; then 
                break
            fi
        done
    fi
    return $OK
}
function remove_downtime
{
    OK=0
    USE_DOWNTIME=1
    if [[ $USE_DOWNTIME -ne 0 ]]; then
        for host in $(curl -ks "http://c.yandex-team.ru/api/groups2hosts/media-jmon") ; do 
            echo  "Trying to remove downtime from host at $(hostname)"
            curl -ks "http://${host}:8998/api/downtimes/remove_downtime?do=1&object_name=$(hostname)"
            curl -ks "http://${host}:8998/api/downtimes/remove_downtime?do=1&object_name=kp-front&service_name=500-http-nginx-rating"
            curl -ks "http://${host}:8998/api/downtimes/remove_downtime?do=1&object_name=kp-backend&service_name=haproxy"
            OK=$?
            if [[ ${OK} == 0 ]]; then 
                break
            fi
        done
    fi
    return $OK
}


if [ "$1" == "--now" ]; then

	downtime_host
	bk_list="$(curl http://c.yandex-team.ru/api/groups2hosts/kp-master 2>/dev/null) $(curl http://c.yandex-team.ru/api/groups2hosts/kp-conv 2>/dev/null) $(curl http://c.yandex-team.ru/api/groups2hosts/kp-backend-prestable 2>/dev/null)  $(curl http://c.yandex-team.ru/api/groups2hosts/kp-backend 2>/dev/null)"
	echo "close backends:"
	for i in $bk_list; do
		$IPT -A INPUT -s $i -p tcp -m multiport --dports 9998,9999,3306 -j REJECT
		echo -n "."
        	sleep 10
	done
	echo -e "\nDone" 
	sleep 15 
	echo "waiting closed all connection:"
	while : ; do
        	CONNS=`mysql -e 'show processlist'|grep handlersocket|awk -F":" '{print $2}' | awk '{print $2}' | awk '{ sum += $1; } END { print sum; }'`
        	echo -n  $CONNS 
    		echo -n  " "
        	[[ $CONNS -gt 0 ]] || break
        	sleep 10
    	done

	echo -e  "\nstart truncate"
	mysql cache < /usr/share/hs-clear/truncate.sql
	echo -e "truncate finished"
	sleep 5
	echo "open backends:"
	for i in $bk_list; do
		$IPT -D  INPUT 1
		echo -n "."
		sleep 10
	done;
	echo -e "\nDone"
	remove_downtime
	sleep 1800

else
        ds=$(${DU} -s /opt/mysql/cache | awk '{print $1}')
        ds=$((ds / 1024 / 1024))
        if [ $ds -gt "${CLEAN_THRESHOLD}" ]; then
                echo "Clean is need. Random sleeping"
                number=$RANDOM
                let "number %= $MAX_SLEEP"
                sleep $number
                echo "Try lock clean"
                ${ZK_FLOCK} $ZK_KEY "$0 --now"
        fi
fi
