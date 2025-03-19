#!/bin/sh

YAV_TOKEN=`cat ~/.ssh/yav_token`

# Determine temp directory
SECRETS_PATH="tmp-$(date +%s)"

# Create temp dir
mkdir ${SECRETS_PATH}

# Get service account key file from yav
ya vault get version sec-01emkgbng63e3n5k7r481dj35n -o file > $SECRETS_PATH/sa-key.json

# Run terraform
declare -a TARGET_ARG
TARGET_ARG=("-target" "yandex_vpc_security_group.common-security-group")

terraform $* "${TARGET_ARG[@]}" -var yc_service_account_key_file=$SECRETS_PATH/sa-key.json -var yandex_token=${YAV_TOKEN}

# Delete temp dir
rm -rf $SECRETS_PATH
