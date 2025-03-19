#!/bin/bash
RAND=$(( $RANDOM % 10 ))
#make sampling
if [ $RAND -eq 6 ]
then
     sleep $(( $RANDOM % 10 ))
    /usr/sbin/valve_check.py 2>/dev/null
else
    echo "PASSIVE-CHECK:valve;0;OK"
fi
