#!/bin/bash

BASE=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
LOCK=/var/lock/ib-mon-export-switch-state.lock
LOG_DIR=/var/log/ib-mon-export-switch-state

tty -s || exec 2>$LOG_DIR/export-switch-state.err

[ -n "`which manage_the_unmanaged 2>/dev/null`" ] || exit 0

[ -f $LOCK ] || touch $LOCK

log=$LOG_DIR/switch-state.$(date +%Y-%m-%d-%H-%M-%S).$$.log
(
	flock --nonblock 200 || exit 0
	$BASE/is-master.sh || exit 0

	python3 $BASE/switch-info.py | tee $log

	find $LOG_DIR -mmin +$((60*24*7)) -type f -name 'switch-state*bz2' -delete || :
	bzip2 --keep $log && mv -f $log $LOG_DIR/switch-state.log || :
) 200<$LOCK

