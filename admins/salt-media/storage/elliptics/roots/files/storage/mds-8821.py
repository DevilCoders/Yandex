#!/bin/bash

state_file='/var/tmp/mds-8821.state'
error_msg='defrag: merge: number of items in merged chunk diverged from stats'
timetail -n 400 -t java /var/log/elliptics/node-1.log | grep "$error_msg" >> $state_file

if [ -s $state_file ]
then
	count=`wc -l $state_file | awk '{print $1}'`
	echo "2;$count errors in $state_file"
else
	echo "0;Ok"
fi
