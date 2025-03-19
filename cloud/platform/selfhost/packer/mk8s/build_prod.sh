#!/usr/bin/env bash

# Usage example:
# COMMIT_REVISION="6039347" \
# DESCRIPTION="build mk8s-controller image on paas-base" \
# YC_TOKEN="$YC_MY_OAUTH_TOKEN" \
# SSH_PRIVATE_KEY_FILE="~/.ssh/ya_id_rsa" \
# ./build_prod.sh

set -euxo pipefail

export YC_FOLDER_ID="b1gbl2vlrvhlj2kcituf" # managed-kubernetes
export YC_SOURCE_FOLDER_ID="b1gkivmq4kv5u4jnqv3g" # build-image-with-packer

export YC_SUBNET_ID="b0c7crr1buiddqjmuhn7"
export YC_ENDPOINT="api.cloud.yandex.net:443"

./build.sh
