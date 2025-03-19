#!/bin/bash

die () {
    echo "$1;$2"
    exit 0
}

count=0
containers=""

while IFS= read -r line
do
    count=$(( count + 1 ))
    containers="$containers $line"
done < <(grep mark-orphan /var/cache/porto_agent_states/ -r 2>/dev/null | grep extra | rev | cut -d/ -f1 | cut -d. -f2- | rev)

if [ "$count" = "0" ]
then
    die 0 "OK"
else
    die 1 "$count orphan containers:$containers"
fi
