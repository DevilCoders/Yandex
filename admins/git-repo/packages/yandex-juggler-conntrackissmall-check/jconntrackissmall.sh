#!/bin/bash

check_conntrack () {
	if [ -e /proc/sys/net/nf_conntrack_max ]; then 
	return 1
	else echo "0; conntrack seems disabled"; return 0
	fi
}

check_chars () {
	connmax=$(cat /proc/sys/net/nf_conntrack_max)
	if [[ ${connmax} -ge 100000 ]]; then echo "0;conn_max=$connmax"
	else 
	   echo "`date +%D\" \"%H:%M:%S` FLAP Value is: $connmax , uptime is: `uptime | awk '{print $3}'`" >> /tmp/jconn.errs
	   sysctl -p 1>/dev/null 2>&1; sleep 2
	   connmax=$(cat /proc/sys/net/nf_conntrack_max)
	   if [[ ${connmax} -ge 100000 ]]; then 
	       echo "0;conn_max=$connmax"
	   else
	       echo "2; conntrack_max=$connmax, sysctl -p not worked"
	   fi
	fi
}

check_conntrack || check_chars


