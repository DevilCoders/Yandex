#!/usr/bin/env bash
set -eo pipefail

CLOUD=aoe0oie417gs45lue0h4

ycp --profile=preprod resource-manager folder list --cloud-id $CLOUD | yq -c .[] | while read -r FOLDER; do
  FOLDER_ID="$(yq -r .id <<< $FOLDER)"
  echo =============================
  echo FOLDER $FOLDER_ID $(yq .name <<< "$FOLDER")
  ycp --profile=preprod load-balancer network-load-balancer list --folder-id "$FOLDER_ID" < /dev/null | yq -c .[] | while read -r L3; do
    L3_ID="$(yq -r .id <<< $L3)"
    yq -c '[.id, .name, .attached_target_groups[]?.health_checks[]?.http_options]' <<< "$L3"
    for TG_ID in $(yq -c '.attached_target_groups[]?.target_group_id' <<< "$L3"); do
      echo "
        network_load_balancer_id: $L3_ID
        target_group_id: $TG_ID
      " | ycp --profile=preprod load-balancer network-load-balancer get-target-states -r- \
        |  yq -c '.target_states[]|del(.subnet_id)' | sort
    done
  done
done
