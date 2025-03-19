#!/bin/bash

BASE=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
LOCK=/var/lock/ib-mon-export-metrics.lock
LOG_DIR=/var/log/ib-mon-export-metrics

[ -f $LOCK ] || touch $LOCK

tty -s || exec 2>$LOG_DIR/export-metrics.err

log=$LOG_DIR/export-metrics.$(date +%Y-%m-%d-%H-%M-%S).$$.log
(
	flock --nonblock 200 || exit 0

	if $BASE/is-master.sh; then
		echo "infiniband,host=`hostname -f` ufm_master=1,ufm_slave=0"
		python3 $BASE/parser.py
		[ -f $BASE/var/no-ufm ] || python3 $BASE/ufm-switch-info.py --modules
		[ -f $BASE/var/no-ufm ] || python3 $BASE/ufm-switch-info.py --temperature
	else
		echo "infiniband,host=`hostname -f` ufm_master=0,ufm_slave=1"
	fi | tee $log

	find $LOG_DIR -mmin +$((60*24*7)) -type f -name 'export-metrics*bz2' -delete || :
	bzip2 --keep $log && mv -f $log $LOG_DIR/export-metrics.log || :
) 200<$LOCK

