#!/bin/bash
dnet_client backend -r `hostname -f`:1025:10 --backend 10 defrag
while [ `dnet_client backend -r \`hostname -f\`:1025:10 status | egrep defrag_state | egrep -v 0 | wc -l` -ne 0 ]
do
    sleep 5
done

