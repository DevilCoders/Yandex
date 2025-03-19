#!/bin/bash

unknown_time () {
echo "2; unknown last sync time"
exit 1
}

set_var () { 
touch /var/log/dndl-sync-ng.last
lastsynctime=$(cat /var/log/dndl-sync-ng.last)
test -z $lastsynctime && unknown_time
nowtime=$(date +%s)
lastsync=$(echo $[$nowtime-$lastsynctime])
}

echo_ok () {
    echo "0; ok, $lastsync sec"
    return 0
}

echo_error () {
    echo "2; $lastsync sec"
    return 1
}

check_lastsync () {
    set_var
    if [[ ${lastsync} -ge 600 ]]; then
	echo_error
	else echo_ok
    fi
}

check_lastsync
