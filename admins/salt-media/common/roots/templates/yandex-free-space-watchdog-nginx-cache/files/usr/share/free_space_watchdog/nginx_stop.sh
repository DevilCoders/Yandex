#!/bin/bash
# MANAGED BY SALT
# DEVELOPED BY PAULSTRONG
export PATH=/usr/local/bin:/usr/local/sbin:/bin:/sbin:/usr/bin:/usr/sbin
service=service-nginx-stop
dir=/var/log/free_space_watchdog
mkdir -p $dir
log=$dir/$service.log
echo -e "tskv\ttimestamp=`date +%FT%T`" | tee -a $log
service nginx stop
