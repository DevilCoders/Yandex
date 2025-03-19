#!/bin/bash

if [ -z $1 ] ; then echo "Usage: $0 hostname_to_ping" ; exit 0 ; fi
HOST=$1
stat=`ping -c3 -W3 $HOST |  grep received | awk '{print $4}' | grep 3`
if [ $stat -eq 3 ] ; then echo '0; OK' ; else echo "2; ping $HOST failed" ; fi

