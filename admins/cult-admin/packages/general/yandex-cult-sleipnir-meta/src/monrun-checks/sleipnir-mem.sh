#!/bin/bash

die () {
	echo "$1;$2"
	exit 0
}

exit_code=$(/usr/bin/sleipnir -u mem run &>/dev/null; echo $?)

if [ "x$exit_code" == "x32" ]; then
	die 0 "OK, virtual_host, skip checking"
elif [ "x$exit_code" == "x0" ]; then 
	die 0 "OK"
else
	die 2 "CRIT"
fi
