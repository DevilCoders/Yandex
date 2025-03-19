{%- set interfaces = salt['underlay.interfaces']() -%}
{%- set hostname = grains['nodename'] -%}
{%- set upstream = salt['grains.get']('cluster_map:hosts:%s:slb_adapter:upstream' % hostname)  -%}
#!/bin/bash

# wait for two seconds for icmp pong sent via overlay to arrive
ping6 -w 2 -n -c 1 -I {{ interfaces['overlay'] }} {{ upstream }} &>/dev/null

status=$?
if [[ "$status" -eq 0 ]]; then
   echo "PASSIVE-CHECK:upstream-available;0;OK"
else
   echo "PASSIVE-CHECK:upstream-available;1;ping6 failed for upstream {{ upstream }}"
fi
