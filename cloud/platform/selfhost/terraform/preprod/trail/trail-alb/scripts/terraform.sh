#!/bin/sh

YAV_TOKEN=`cat ~/.ssh/yav_token`

MODULE_NAME="control-plane-instance-group"

declare -a TARGET_ARG
IFS=","
read -a arr <<< "$TARGET"
for val in "${arr[@]}"; do
  TARGET_ARG=(${TARGET_ARG[@]} "-target" "module.$MODULE_NAME.ycp_compute_instance.node[$val]")
done

terraform $* "${TARGET_ARG[@]}" -var yandex_token=${YAV_TOKEN}


