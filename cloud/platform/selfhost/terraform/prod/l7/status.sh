#!/usr/bin/env bash

function status {
  # Usage: status what L3_id TG_id IG_id
  TARGETS=$(echo "
    network_load_balancer_id: $2
    target_group_id: $3
  " | ycp load-balancer network-load-balancer get-target-states "${@:5}" -r- \
    |  yq -c '.target_states|sort_by(.address)|.[]|del(.subnet_id)')
  INSTANCES=$(ycp microcosm instance-group list-instances "$4" "${@:5}" \
    | yq -c 'map([.status, .network_interfaces[].primary_v6_address.address, .status_message // "No error"])|sort_by(.[1])|.[]')
  echo "$1 (IG $4)"
  paste <(echo "$TARGETS") <(echo "$INSTANCES")
}

#                 L3                   L3 TG                IG
status cpl-router b7rrqpami0of73r8dfs0 b7rkmtrf9bhc9up8tvlc cl1qihmkhh0ste9aot6m --profile=prod
status api-router b7r1v0ccl9hgui4md4p5 b7rf6r4nqjg3n4bi5ulk cl1iddc59ha3irr18lhi --profile=prod
