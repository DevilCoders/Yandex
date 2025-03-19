#!/bin/bash

YUB="yandex-udp-balancer"
YUB_ERRORS=$(sudo journalctl -u yandex-udp-balancer --since "15 minutes ago" -q | grep -vP 'ERROR.*Check URL'| grep ERROR -c )
YUB_CHECK_ERRORS=$(sudo journalctl -u yandex-udp-balancer --since "15 minutes ago" -q | grep -P 'ERROR.*Check URL' | awk 'match($0, /http:\/\/\w{3}-matilda0\d*/) { print substr( $0, RSTART+7, RLENGTH-6 ) }' | sort | uniq | wc -l)

if [ $YUB_ERRORS -eq 0 ]; then
    echo "PASSIVE-CHECK:"$YUB"-errors;0;OK"
elif [ $YUB_ERRORS -lt 10 ]; then
    echo "PASSIVE-CHECK:"$YUB"-errors;1;Got $YUB_ERRORS errors in log file"
else
    echo "PASSIVE-CHECK:"$YUB"-errors;2;Got $YUB_ERRORS errors in log file"
fi

if [ $YUB_CHECK_ERRORS -eq 0 ]; then
    echo "PASSIVE-CHECK:"$YUB"-connection-errors;0;OK"
elif [ $YUB_CHECK_ERRORS -lt 3 ]; then
    echo "PASSIVE-CHECK:"$YUB"-connection-errors;1;$YUB_CHECK_ERRORS nodes inaccessible"
else
    echo "PASSIVE-CHECK:"$YUB"-connection-errors;2;$YUB_CHECK_ERRORS nodes inaccessible"
fi


for SERVICE in $(dpkg -L matilda-server python3-frontilda | grep service | cut -d / -f 5 | cut -d . -f 1); do
    if systemctl is-active matilda-ipfix >/dev/null; then
        echo "PASSIVE-CHECK:"$SERVICE"-alive;0;OK"
    else
        echo "PASSIVE-CHECK:"$SERVICE"-alive;1;Failed"
    fi
done
