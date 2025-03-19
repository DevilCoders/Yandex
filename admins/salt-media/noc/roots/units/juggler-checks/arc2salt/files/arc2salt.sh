#!/bin/bash

###
check_prefix="PASSIVE-CHECK:arc-mounted;"
if ! mount|grep ^arc > /dev/null; then
    echo "${check_prefix}2; Arc is not mounted"
else
    echo "${check_prefix}0; Ok"
fi

###
check_prefix="PASSIVE-CHECK:arc2sync_last_update;"
MODIFY_TS=$(stat --format %Y /var/cache/salt/arc2salt.json 2>/dev/null || echo 0)
LAG=$(($(date +%s) - $MODIFY_TS))

if [ $LAG -gt 3600 ];then
    echo "${check_prefix}2; arc2salt is out of sync: last update" $(($LAG/3600)) "hours ago"
else
    echo "${check_prefix}0; Ok"
fi
