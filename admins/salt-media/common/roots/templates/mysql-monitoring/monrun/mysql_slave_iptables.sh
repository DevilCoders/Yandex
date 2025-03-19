#!/bin/bash

CHECK=$(curl -s "http://localhost:4417/mon/replica" || curl -s "http://localhost:2080/mon/replica")
IPV4_RULES=$(sudo iptables -w -L -v -n 2>/dev/null | grep '3306')
IPV6_RULES=$(sudo ip6tables -w -L -v -n 2>/dev/null | grep '3306')
IPV4_STACK=$(ip -4 a l | grep -v "127.0.0.1" | grep "inet")
IPV6_STACK=$(ip -6 a l | grep -v "::1" | grep "inet6")
ERROR_CODE=$(echo "$CHECK"|awk -F";" '{print $1}')
SEC_BEHIND=$(echo "$CHECK"|awk -F";" '{print $2}'|grep -Po '\d+')
TIME_TO_WATCH=$1
IP_RULES="present"
RE='^[0-9]+$'


if [[ -n "$IPV4_STACK" && -z "$IPV4_RULES" ]];then
    IP_RULES="empty"
fi

if [[ -n "$IPV6_STACK" &&  -z "$IPV6_RULES" ]];then
    IP_RULES="empty"
fi


# Sometimes check sends garbage.
if ! [[ "$ERROR_CODE" =~ $RE ]];then
    echo "1; Received garbage from mysql replica check"
    exit
fi

if [[ "$ERROR_CODE" -eq "0" || "$ERROR_CODE" -eq "1" ]];then
    echo "$CHECK"
    exit
fi


if [[ "$ERROR_CODE" -eq "2" && "$SEC_BEHIND" == "stopped" ]];then
    echo "1;replica has stopped"
    exit
fi

if [ "$ERROR_CODE" -eq "2" ];then
    if [ "$SEC_BEHIND" -ge "$TIME_TO_WATCH" ];then
        if [ "$IP_RULES" == "empty" ];then
            echo "2;iptables rules missing"
        else
            echo "0;iptables rules present. Slave lag $SEC_BEHIND sec."
        fi
    else
        echo "0;Slave lag $SEC_BEHIND sec."
    fi
fi

