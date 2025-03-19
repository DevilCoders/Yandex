#!/bin/bash
if [ `find /run/lock/elliptics_merge.lock -type f -mtime -1 | wc -l` == 0 ]
then
    zk-flock ape_storage_merge -c /etc/distributed-flock-media.json "/bin/bash /usr/local/bin/elliptics_merge.sh"
fi
