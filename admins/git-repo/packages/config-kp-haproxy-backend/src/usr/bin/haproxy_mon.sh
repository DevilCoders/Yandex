#!/bin/sh
p=$(echo "show stat" | socat unix-connect:/tmp/haproxy_stat stdio 2>/dev/null)
if [ $? -ne 0 ]; then echo "2; haproxy down"; exit; fi
n=$(echo "$p" | grep DOWN  | wc -l)
if [ "$n" -eq "0" ]; then echo "0; OK"; else echo "2; ${n} upstreams down"; fi
