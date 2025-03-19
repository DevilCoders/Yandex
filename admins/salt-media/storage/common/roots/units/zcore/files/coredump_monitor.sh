#!/bin/bash
min_coredumps=$1
if [[ $min_coredumps == "" ]] #default_value
	then min_coredumps=0
fi
if [ `ls -1 \`sysctl -a 2>/dev/null | grep kernel.core_pattern | awk '{print $NF}' | sed 's/\..*//g;s/$/*/g'\` 2>/dev/null | wc -l` -gt "$min_coredumps" ]
        then echo "2; `ls -1 \`sysctl -a 2>/dev/null | grep kernel.core_pattern | awk '{print $NF}' | sed 's/\..*//g;s/$/*/g'\` | wc -l` coredumps found"
        else echo "0;OK"
fi
exit 0
