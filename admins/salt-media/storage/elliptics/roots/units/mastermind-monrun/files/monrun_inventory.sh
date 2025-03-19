#!/bin/bash

# FIX ALL
update_period=`grep cache_update_period /etc/elliptics/mastermind.conf | grep -Po '\d+' | sort -nr | head -n1`
if [ -z $update_period ]
then
    update_period=600
fi
collector_update_period=120

tmp_file=`mktemp`
timetail -t java -n `echo "$update_period * 5" | bc` /var/log/cocaine-core/file.*.log | grep 'mm.inventory' > $tmp_file
bad_host=`cat $tmp_file | grep -Po "(?<=Failed to get inventory host nodes for host ).*" | sort | uniq -c | sort -nk 1`
cloud_bad_hosts_message=`echo "$bad_host" | awk '{print $2" ("$1"),"}' | tr '\n' " "`
cloud_errors=`echo "$bad_host" | tail -n1 | awk '{print $1}'`

# /var/log/mastermind/mastermind-collector.log
# 2019-11-27 13:03:16.162717 0c3192b05dde49d5/43612/26408 ERROR: Collection failed: N7collect12CollectErrorE: Nodes: Inventory: Cocaine call failed, attrs: [trace_id: 0c3192b05dde49d5]
timetail -t java -n `echo "$update_period * 5" | bc` /var/log/mastermind/mastermind-collector.log | grep -P 'ERROR: Inventory has no info about host|Inventory: Cocaine call failed' > $tmp_file
bad_host=`cat $tmp_file | grep -Po "(?<=ERROR: Inventory has no info about host ).*(?=, attrs)" | sort | uniq -c | sort -nk 1`
inventory_timeout=`cat $tmp_file | grep -o "Inventory: Cocaine call failed" | tail -n1`

collector_bad_hosts_message=`echo "$bad_host" | awk '{print $2" ("$1"),"}' | tr '\n' " "`
collector_errors=`echo "$bad_host" | tail -n1 | awk '{print $1}'`
if [ ! -z "$inventory_timeout" ]
then
    collector_errors=$((collector_errors + 4))
    collector_bad_hosts_message="$collector_bad_hosts_message $inventory_timeout"
fi

rm $tmp_file

if [ ! -z "$cloud_errors" ] && [ $cloud_errors -gt 3 ]
then
    echo "2; failed to get inventory for (cloud: $cloud_bad_hosts_message)"
    exit 0
fi

if [ ! -z "$collector_errors" ] && [ $collector_errors -gt 3 ]
then
    echo "2; failed to get inventory for (collector: $collector_bad_hosts_message)"
    exit 0
fi

echo "0; Ok"
