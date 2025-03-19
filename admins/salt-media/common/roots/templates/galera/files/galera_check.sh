#!/bin/bash

#############################################################################
# determine configured garbd on host
tmpfile="/tmp/__galera_check"
if [[ -e $tmpfile ]]
then
    my_cgroup=$(cat $tmpfile)
else
    me=$(hostname -f)
    my_cgroup=$(curl -s "https://c.yandex-team.ru/api-cached/hosts2groups/$me" | tail -1)
    if [[ $? -ne 0 ]]
    then
        echo "2; Failed to get my conductor group"
        exit 0
    else:
        echo $my_cgroup > $tmpfile
    fi
fi
# don't continue if garbd config found and our group specified there
if [[ -e /etc/default/garbd ]]
then
    grep -sq $my_cgroup /etc/default/garbd >/dev/null
    if [[ $? -eq 0 ]]
    then
        /usr/sbin/service garbd status >/dev/null
        if [[ $?  -ne 0 ]]
        then
            echo "2; Galera Arbitrator is down"
            exit 0
        else
            echo "0;OK: This is configured Galera Arbitrator host"
            exit 0
        fi
    else
        echo "1;This is not configured Galera Arbitrator host"
        exit 0
    fi
fi
#############################################################################
# checking cluster state got from yandex-common-monitoring-galera (Maintainer: Mikhail Petrov <azalio@yandex-team.ru>)
STATUS=$(mysql --defaults-file=/etc/mysql/client.cnf -NBe "show status like 'wsrep_%';" 2>/dev/null | egrep '(wsrep_local_state_comment|wsrep_local_send_queue_avg|wsrep_local_recv_queue_avg|wsrep_flow_control_paused|wsrep_flow_control_sent|wsrep_ready|wsrep_cluster_status|wsrep_connected)')

if [[ -z $STATUS ]]; then
        echo "PASSIVE-CHECK:galera_cluser;2;MySQL is down"
        exit 0
fi

declare -A wsrep

while read line; do
        read var value <<<$(echo $line)
        wsrep[$var]=$value
done < <(echo "$STATUS")

WARNING=""
ERROR=""
st=0

if [[ ${wsrep[wsrep_local_state_comment]} != "Synced" ]]; then
        WARNING="wsrep_local_state_comment is ${wsrep[wsrep_local_state_comment]}"
fi
if [[ ${wsrep[wsrep_ready]} != "ON" ]]; then
        ERROR="wsrep_ready is ${wsrep[wsrep_ready]}; "
fi
if [[ ${wsrep[wsrep_cluster_status]} != "Primary" ]]; then
        ERROR+="wsrep_cluster_status is ${wsrep[wsrep_cluster_status]}"
fi

if [[ ${wsrep[wsrep_connected]} != "ON" ]]; then
        ERROR+="wsrep_connected is ${wsrep[wsrep_connected]}"
fi

if [[ -n $ERROR ]]; then
        st=2
fi
if [[ -n $WARNING ]]; then
        st=1
fi

#############################################################################
if [[ $st -gt 0 ]]
then
    if [[ ! -f /tmp/close_galera_node ]]
    then
        touch /tmp/close_galera_node
        echo "2;node closed"
    else
	echo "2;node closed"
    fi
fi

if [[ $st -eq 0 ]]
then
    if [[ -f /tmp/close_galera_node ]]
    then
	rm /tmp/close_galera_node
        echo "0;OK: node open"
    else
	echo "0;OK: node open"
    fi
fi
