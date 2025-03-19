#!/bin/bash

die () {
	echo "PASSIVE-CHECK:ntp_stratum;$1;$2"
	exit 0
}

# get current stratum
ntp_status=`/usr/sbin/ntpdate -t 1 -q 127.0.0.1 2>&1`
exit_code=$?
stratum=`echo $ntp_status | grep stratum | sed -e 's/.*stratum \([0-9]\+\).*/\1/'`

if [ "$exit_code" -ne "0" ]; then
	die 2 "Local ntp stratum is $stratum"
else
	die 0 "OK"
fi
