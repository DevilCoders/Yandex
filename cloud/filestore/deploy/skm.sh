#!/usr/bin/env bash

PROFILE=${1?Profile should be set as the first argument}
shift 1

if [[ "$PROFILE" != "prod" && "$PROFILE" != "preprod" ]]; then
    echo "invalid profile specified:" $PROFILE
    exit 1
fi

SECRETS_DIR="$(dirname "$0")/secrets/$PROFILE"
pushd $SECRETS_DIR

YAV_TOKEN=$(cat ~/.yav/token) \
YC_TOKEN=$(yc iam create-token --profile=$PROFILE) \
skm $@

popd
