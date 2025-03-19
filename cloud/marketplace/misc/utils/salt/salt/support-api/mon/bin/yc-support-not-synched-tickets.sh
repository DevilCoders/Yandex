#!/bin/bash
name='not_synched_tickets'
health_url="http://localhost:5001/juggler_checks/not_synched_tickets"

check=$(/usr/bin/curl -fs ${health_url})

if [[ $check ]] ; then
    echo "$check"
else
    echo "PASSIVE-CHECK:$name;2;Down" ; exit
fi

