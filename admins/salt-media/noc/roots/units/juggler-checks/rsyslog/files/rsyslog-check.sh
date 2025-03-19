#!/bin/bash

if systemctl is-active rsyslog > /dev/null
then
    date +%s|logger -n `hostname -f` -t rsyslog-juggler-check
    if [ -f /var/log/rsyslog/checks.log ]
    then
        lag=$(sudo grep rsyslog-juggler-check /var/log/rsyslog/checks.log|tail -n1|awk '{print strftime("%s")-$NF}')
        if [ $lag -lt 600 ]
        then
            echo "PASSIVE-CHECK:rsyslog;0;Lag is $lag"
        elif [ $lag -lt 1200 ]
        then
            echo "PASSIVE-CHECK:rsyslog;1;Lag is $lag"
        else
            echo "PASSIVE-CHECK:rsyslog;2;Lag is $lag"
        fi
    else
        echo "PASSIVE-CHECK:rsyslog;0;Ok"
    fi
else
    echo "PASSIVE-CHECK:rsyslog;2;rsyslog is down"
fi
