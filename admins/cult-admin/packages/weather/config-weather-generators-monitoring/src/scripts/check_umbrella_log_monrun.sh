#!/bin/bash
LOG="/tmp/umbrella-errcont.tmp"

if [[ -f "$LOG" ]]; then
	COUNT=`cat $LOG`
	if [[ "$COUNT" -lt "100" ]]; then
		echo "0;OK. Count error last hour $COUNT"
	else
		echo "2; Count error last hour $COUNT"
	fi
else
	echo "2;Data not stores!"
fi

