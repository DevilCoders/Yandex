#!/usr/bin/env bash

PROFILE=${1?Profile should be set as the first argument}
shift 1

if [[ "$PROFILE" != "prod" && "$PROFILE" != "preprod" ]]; then
    echo "invalid profile specified:" $PROFILE
    exit 1
fi

TERRAFORM_DIR="$(dirname "$0")/terraform/$PROFILE"
pushd $TERRAFORM_DIR

terraform plan $@ \
    -var token="$(yc iam create-token --profile=$PROFILE)"

popd
