#!/bin/bash

ticks_per_sec=$(getconf CLK_TCK)
uptime=$(awk '{print $1}' /proc/uptime | cut -d. -f1)

for pid in $(pgrep qemu)
do
    ticks_since_boot="$(awk '{print $22}' "/proc/$pid/stat" 2>/dev/null)"
    if [ "$ticks_since_boot" = "" ]
    then
        # process already ended
        continue
    fi
    secs_since_boot=$(( ticks_since_boot / ticks_per_sec ))
    run_time=$(( uptime - secs_since_boot ))
    if (( "$run_time" > 7200 ))
    then
        kill "$pid" 2>/dev/null
    fi
done
