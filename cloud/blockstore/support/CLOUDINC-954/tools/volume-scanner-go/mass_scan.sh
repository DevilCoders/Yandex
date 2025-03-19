#!/usr/bin/env bash

nbs_host="sas09-ct5-9.cloud.yandex.net"

mkdir -p scan_logs

i=0
while read line
do
    echo "starting scan for disk $line" &&
    # echo "./blockstore-volume-scanner --disk-id $line --iam-token-file tok --port 9768" > "scan_logs/${line}" && sleep 2 &
    ./blockstore-volume-scanner --host "$nbs_host" --disk-id $line --iam-token-file tok --port 9768 2> "scan_logs/${line}.err" > "scan_logs/${line}.out" &
    ((++i % 30 == 0 )) && wait
done
