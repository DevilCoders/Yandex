#!/bin/bash
while [ 1 ]
do
    curl -s -H 'Host:echo' 'http://localhost:82/' >/dev/null 2>&1
    sleep `echo $RANDOM % 2 + 1 | bc`
done

