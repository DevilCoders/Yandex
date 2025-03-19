#!/bin/bash

while read line; do
    echo $line
    ./blockstore-client executeaction \
        --port 9766 \
        --host localhost \
        --action ResetTablet \
        --input-bytes '{"TabletId": "'"$line"'", "Generation": 0}'
done < another_dead_tabs.txt 2> reset2_log.txt > reset2_out.txt
