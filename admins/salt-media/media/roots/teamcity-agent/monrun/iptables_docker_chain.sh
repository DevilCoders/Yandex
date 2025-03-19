#!/bin/bash

IPTABLES=/sbin/iptables
IP6TABLES=/sbin/ip6tables

ISIPV4=$(ip r | grep default | wc -l)
ST="0"
MSG=""


function check_mascq()
{
    masq6=$(sudo $IP6TABLES -w -t nat -L POSTROUTING | grep MASQUERADE | wc -l)

    if [[ $ISIPV4 -gt 0 ]]
    then
        masq4=$(sudo $IPTABLES -w -t nat -L POSTROUTING | grep MASQUERADE | wc -l)
        if [[ $masq4 -eq 0 ]]
        then
            MSG="${MSG} (ipv4) No MASQUERADE rule in table nat chain POSTROUTING."
            ST="2"
        fi
    fi

    if [[ $masq6 -eq 0 ]]
    then
        MSG="${MSG} (ipv6) No MASQUERADE rule in table nat chain POSTROUTING."
        ST="2"
    fi

    if [[ $ST -eq 0 ]]
    then
        MSG="OK"
    fi

    return 0
}

check_mascq
echo "${ST};${MSG}"
exit 0
