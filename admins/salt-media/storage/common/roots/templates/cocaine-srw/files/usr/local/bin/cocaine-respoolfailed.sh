#!/bin/bash

sleep $((RANDOM % 900))
for i in $(/usr/local/bin/cocaine-depth.py | grep -o broken.* | awk '{print $2}' | sed 's/,/\n/g' | sed '$d'); do cocaine-tool app restart -n $i --profile $(cocaine-tool runlist view -n $(grep runlist /etc/cocaine/cocaine.conf  | tr ":","\"" " " | awk '{print $NF}') --timeout=90| grep $i | cut -d\" -f4) --timeout=240; done 2>/dev/null >/dev/null

