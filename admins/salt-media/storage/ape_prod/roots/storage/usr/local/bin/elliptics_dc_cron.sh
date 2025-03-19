#!/bin/bash
if [ `find /run/lock/elliptics_dc.lock -type f -mtime -7 | wc -l` == 0 ]
then
    zk-flock ape_storage_dc -c /etc/distributed-flock-media.json "/bin/bash /usr/local/bin/elliptics_dc.sh"
fi
