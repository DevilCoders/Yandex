#!/usr/bin/env bash

# Usage example:
# COMMIT_REVISION="6039347" \
# DESCRIPTION="build mk8s-controller image on paas-base" \
# YC_TOKEN="$YC_MY_OAUTH_TOKEN" \
# SSH_PRIVATE_KEY_FILE="~/.ssh/ya_id_rsa" \
# ./build_preprod.sh

set -euxo pipefail

export YC_FOLDER_ID="aoe56gngo7aoaa45nrga" # managed-kubernetes
# export YC_SOURCE_FOLDER_ID="aoe824hvnc67es4f8kqj" # paas-images
export YC_SOURCE_FOLDER_ID="aoer21p20u9f8qb3teu0" # build-image-with-packer

export YC_SUBNET_ID="fo27jfhs8sfn4u51ak2s"
export YC_ENDPOINT="api.cloud-preprod.yandex.net:443"

./build.sh
