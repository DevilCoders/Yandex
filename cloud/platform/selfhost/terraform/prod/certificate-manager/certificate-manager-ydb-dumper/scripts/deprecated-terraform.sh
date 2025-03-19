#!/bin/sh

YAV_TOKEN=`cat ~/.ssh/yav_token`

# Determine temp directory
SECRETS_PATH="tmp-$(date +%s)"

# Create temp dir
mkdir ${SECRETS_PATH}

# Get service account key file from yav
ya vault get version sec-01emkym7m86ma5bxcp5sbc1gvg -o file > $SECRETS_PATH/sa-key.json

# Run terraform
MODULE_NAME="certificate-manager-ydb-dumper-instance-group"

declare -a TARGET_ARG
IFS=","
read -a arr <<< "$TARGET"
for val in "${arr[@]}"; do
  TARGET_ARG=(${TARGET_ARG[@]} "-target" "module.$MODULE_NAME.ycp_compute_instance.node[$val]")
done

terraform $* "${TARGET_ARG[@]}" -var yc_service_account_key_file=$SECRETS_PATH/sa-key.json -var yandex_token=${YAV_TOKEN}

# Delete temp dir
rm -rf $SECRETS_PATH
