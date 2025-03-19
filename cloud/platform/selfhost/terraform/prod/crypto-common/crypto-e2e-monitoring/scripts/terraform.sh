#!/bin/sh

YC_TOKEN=`cat ~/.ssh/yc_token`
YAV_TOKEN=`cat ~/.ssh/yav_token`

MODULE_NAME="e2e-monitoring-instance-group"

declare -a TARGET_ARG
if [[ -z $TARGET ]]; then
  echo "TARGET must be specified: TARGET= ./scripts/terraform.sh\nExiting"
  exit 1
else
  IFS=","
  read -a arr <<< "$TARGET"
  for val in "${arr[@]}"; do
    TARGET_ARG=(${TARGET_ARG[@]} "-target" "module.$MODULE_NAME.ycp_compute_instance.node[$val]")
  done
fi

exec terraform $* "${TARGET_ARG[@]}" -var yandex_token=${YAV_TOKEN}
