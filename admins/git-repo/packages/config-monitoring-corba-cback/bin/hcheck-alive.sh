#!/bin/bash

err=0
msg="Ok"

alive=`ps ax | grep /usr/sbin/hcheck-bin | grep -vc grep`;
if [ "$alive" -eq "0" ]
then
    err=2
    msg="Failed"
fi

echo "$err; $msg: $alive hcheck processes found"
