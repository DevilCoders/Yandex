#!/usr/bin/env bash
sleep $((RANDOM % 10))
RESTARTS_NUMBER=5 #Specifying number of unattended restarts
PERIOD="10 minutes ago" #Specifying period as for journalctl
RESULT=''
while getopts p:n: flag
do
    case "${flag}" in
        p) PERIOD=${OPTARG};;
        n) RESTARTS_NUMBER=${OPTARG};;
        *) :
    esac
done

while read count name _; do
    if [ $count -gt "$RESTARTS_NUMBER" ]; then
        RESULT+="$name($count)"
    fi
done < <(journalctl --since "$PERIOD" --no-pager _PID=1 | grep "Scheduled restart job" | awk '{print $6}' | sort | uniq -c)
if [ -z "$RESULT" ]; then
    echo "PASSIVE-CHECK:systemd-flaps;0;OK"
else
    echo "PASSIVE-CHECK:systemd-flaps;2;Services restart too often: $RESULT"
fi
