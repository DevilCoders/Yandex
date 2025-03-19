#!/bin/bash

tmp_file='/var/tmp/wd-mastermind-stat'

function restart () {
    echo "ubic restart mastermind-stat"
    ubic restart mastermind-stat > /dev/null 2>&1
    cat /dev/null > $tmp_file
}

timeout 10 curl -ss "localhost:5000/unistat" >/dev/null 2>/dev/null 
if [ $? -eq 124 ]
then
    restart
fi

sig=`curl -ss "localhost:5000/unistat" | jq '.' | grep scheduler_stat_run_summ -A1 |tail -n1 |grep -Po "\d+"`

if [ -z $sig ]
then
    exit 0
fi

if [ -s $tmp_file ]
then
    last_sig=`tail -n1 $tmp_file`
    if [ $last_sig -eq $sig ]
    then
        c=`wc -l $tmp_file | awk '{print $1}'`
        if [ $c -gt 1 ]
        then
            restart
        else
            echo $sig >> $tmp_file
        fi
    else
        echo $sig > $tmp_file
    fi

else
    echo $sig > $tmp_file
fi
