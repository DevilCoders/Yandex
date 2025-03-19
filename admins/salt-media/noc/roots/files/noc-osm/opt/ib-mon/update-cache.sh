#!/bin/bash
BASE=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
CACHE_LOCK=/var/lock/ib-mon-update_cache.lock
[ -f $CACHE_LOCK ] || touch $CACHE_LOCK

LOG_DIR=/var/log/ib-mon

(
	flock --nonblock 200 || exit 0
	set -e
	if [ "$1" == "--verbose" ]; then
		$BASE/update_cache.py
	else
		$BASE/update_cache.py 2>$LOG_DIR/update_cache.new && mv $LOG_DIR/update_cache.new $LOG_DIR/update_cache.log
	fi
) 200<$CACHE_LOCK  </dev/null 


