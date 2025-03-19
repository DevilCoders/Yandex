#!/bin/bash


set_limits () {
    events=100
    PPBPrefilter=20
    failed_PPBPrefilter=20
    failed_ImportPostHandler=444
    FotkiActivityHandler=3666
    FreezeFeedHandler=333
    default_limit=222
}

get_pumper_list () {
    find /var/tmp/inotify/ -type f | sed 's/^\(.*\)\/[^\/].*$/\1/' | uniq -c | sed 's/\/var\/tmp\/inotify\///g' | sed 's/failed\//failed_/g'
}

run_cycle () {
while read a b; do 
    is_critical $a $b
    if [[ $? -ge 1 ]]; then 
	if [[ $1 != "debug" ]]; then 
	        echo "2; Pumper events - run \"yaru-pumper-juggler.sh debug\" for details"
		    exit 1
		    else 
	        echo "CRIT: $b = $a"
		    return 1
		    fi
    else
	return 0
    fi
done
}

is_critical () {
    event_name=$2
    event_amount=$1
    if [[ $event_name == "" ]]; then
        if [[ $event_amount -ge $event_name ]]; then
            return 1
	        else return 0
	        fi
    else
        if [[ $event_amount -ge $default_limit ]]; then
                return 1
        else return 0
        fi
    fi
}

set_limits
get_pumper_list | run_cycle $1 && echo "0; ok" 

if [[ $1 == "debug" ]]; then 
    echo "Look at /var/tmp/inotify"
fi

