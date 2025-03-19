#!/bin/sh
is_slave || exit 0
MASTER=$(get_master) || exit
rsync --delete -av $MASTER::cvs/* /data/CVSROOT/ > /dev/null 2> /tmp/rsync-master.tmp.log || cat /tmp/rsync-master.tmp.log >&2
