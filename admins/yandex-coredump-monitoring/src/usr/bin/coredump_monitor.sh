#!/bin/bash
min_coredumps=$1
if [[ $min_coredumps == "" ]] #default_value
        then min_coredumps=0
fi

# CADMIN-10281
if [[ -f /etc/yandex/coredump_monitor.conf ]]
        then source /etc/yandex/coredump_monitor.conf
        else pattern=""
fi

if [[ $pattern == "" ]]
        then count_coredumps=`ls -1 \`sysctl -a 2>/dev/null | grep kernel.core_pattern | awk '{print $3}' | sed 's/\..*//g;s/$/*/g'\` 2>/dev/null | wc -l`
        else count_coredumps=`ls -1 \`sysctl -a 2>/dev/null | grep kernel.core_pattern | awk '{print $3}' | sed 's/\..*//g;s/$/*/g'\` 2>/dev/null | grep -v -E $pattern | wc -l`
fi

if [ "$count_coredumps" -gt "$min_coredumps" ]
        then echo "2; $count_coredumps coredumps found"
        else echo "0;OK"
fi

exit 0


