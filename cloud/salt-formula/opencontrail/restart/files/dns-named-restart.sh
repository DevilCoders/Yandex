#!/bin/bash

TIMER=contrail-dns-reload.timer
DNS=contrail-dns
NAMED=contrail-named
RECONFIG=/etc/contrail/dns/named-reconfig.sh

echo "$(date) stopping timer '$TIMER'"
systemctl stop $TIMER

echo "$(date) stopping services '$NAMED', '$DNS'"
systemctl stop $NAMED
systemctl stop $DNS

echo "$(date) starting services"

systemctl start $DNS
echo "$(date) service '$DNS' has been started"

systemctl start $NAMED
echo "$(date) service '$NAMED' has been started"

function zones_count() {
    find /etc/contrail/dns/ -type f -name "*.zone" | wc -l
}

prev_count=0
# Wait for 10 minutes (10*60 seconds) max
for ((try=0; try<60; try++)); do
    curr_count=$(zones_count)
    if [[ $curr_count > 0 && $curr_count == $prev_count ]]; then
        echo "$(date) zones: curr=$curr_count, prev=$prev_count, waiting completed"
        break
    fi
    echo "$(date) zones: curr=$curr_count, prev=$prev_count, waiting (try $try) ..."
    sleep 10
    prev_count=$curr_count
done

echo "$(date) calling '$RECONFIG'"
$RECONFIG
echo "$(date) reconfig completed"

echo "$(date) starting timer '$TIMER'"
systemctl start $TIMER

echo "$(date) all operations completed"
