#!/bin/sh

NLB_ID=etqd58uk6pj8mff1bsrp
TG_ID=a19f8ufbbleug38jk4si
echo "network_load_balancer_id: $NLB_ID\ntarget_group_id: $TG_ID" | ycp load-balancer network-load-balancer get-target-states --profile testing -r @
