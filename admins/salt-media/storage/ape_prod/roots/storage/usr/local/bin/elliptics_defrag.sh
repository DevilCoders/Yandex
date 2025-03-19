#!/bin/bash
dnet_client backend -r `hostname -f`:1025:10 status | grep state.*1 -B 1 | grep backend_id | cut -d\: -f2 | cut -d\, -f1 | while read name
do
    dnet_client backend -r `hostname -f`:1025:10 --backend $name defrag
    while [ `dnet_client backend -r \`hostname -f\`:1025:10 status | egrep defrag_state | egrep -v 0 | wc -l` -ne 0 ]
    do
        sleep 5
    done
    sleep 5
done

