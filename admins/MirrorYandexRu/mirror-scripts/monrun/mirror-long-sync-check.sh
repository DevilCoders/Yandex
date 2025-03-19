#!/bin/bash

# check for *-error files in /var/log/mirror/
COUNT=`find /var/log/mirrors/ -name "*.log" -cmin +720 | wc -l`
if [ $COUNT -gt 5 ]
then
    echo "2; Too old sync"
    exit 2
fi
if [ $COUNT -gt 1 ]
then
    echo "1; Too old sync"
    exit 1
fi
echo "0;ok"
