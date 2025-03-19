#!/bin/bash

BASE=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
LOCK=/var/lock/ib-mon-export-ports.lock
LOG_DIR=/var/log/ib-mon-export-ports

[ -f $LOCK ] || touch $LOCK
tty -s || exec 2>>$LOG_DIR/export-ports.err

log=$LOG_DIR/export-ports.$(date +%Y-%m-%d-%H-%M-%S).$$.log
echo "# exec $0 $*" >$log
(
	flock --wait 180 200 || { echo "cannot obtain lock" >>$log ;   exit 0; }

	python3 $BASE/ufm-get-ports.py $*

	find $LOG_DIR -mmin +$((60*24*7)) -type f -name 'export-ports*bz2' -delete || :
) 200<$LOCK  | tee -a $log

bzip2 --keep $log && mv -f $log $LOG_DIR/export-ports.log || :

