#!/bin/sh

YC_TOKEN=`cat ~/.ssh/yc_token`
YAV_TOKEN=`cat ~/.ssh/yav_token`

MODULE_NAME="kms-devel-hsm-instance-group"

declare -a TARGET_ARG
IFS=","
read -a arr <<< "$TARGET"
for val in "${arr[@]}"; do
  TARGET_ARG=(${TARGET_ARG[@]} "-target" "module.$MODULE_NAME.ycp_compute_instance.node[$val]")
done

exec terraform $* "${TARGET_ARG[@]}" -var yc_token=${YC_TOKEN} -var yandex_token=${YAV_TOKEN}
