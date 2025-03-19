#!/usr/bin/env bash

PROFILE=${1?Profile should be set as the first argument}
shift 1

if [[ "$PROFILE" != "prod" && "$PROFILE" != "preprod" ]]; then
    echo "invalid profile specified:" $PROFILE
    exit 1
fi

APP_VERSION=${1?Version should be set as the second argument}
shift 1

if [[ $APP_VERSION =~ ([0-9,a-f,A-F]*)\.trunk(\.([0-9]+))? ]]; then
    REVISION=${BASH_REMATCH[1]}
    BRANCH="trunk"
    TASKID=${BASH_REMATCH[3]}
elif [[ $APP_VERSION =~ ([0-9,a-f,A-F]*)\.(.*)\.(stable-[0-9]+-[0-9]+(-[0-9]+)?)(-([0-9]+))?(\.([0-9]+))? ]]; then
    REVISION=${BASH_REMATCH[1]}
    BRANCH=${BASH_REMATCH[3]}
    HOTFIX=${BASH_REMATCH[6]}
    TASKID=${BASH_REMATCH[8]}
else
    echo "invalid version specified:" $APP_VERSION
    exit 1
fi

if [[ ! $HOTFIX -eq 0 ]]; then
    IMAGE_VERSION=$BRANCH-hotfix$HOTFIX
else
    IMAGE_VERSION=$BRANCH-r$REVISION
fi

PACKER_DIR="$(dirname "$0")/packer"

packer build $@ \
    -var token="$(yc iam create-token --profile=$PROFILE)" \
    -var app_version="$APP_VERSION" \
    -var image_version="$IMAGE_VERSION" \
    -var-file "$PACKER_DIR/variables-$PROFILE.json" \
    "$PACKER_DIR"
