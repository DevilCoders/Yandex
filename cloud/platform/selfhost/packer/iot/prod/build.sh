#!/bin/bash

YC_TOKEN=${YC_TOKEN}
YT_TOKEN=${YT_TOKEN}

if [[ -z "$1" ]]; then
  echo "Usage: build.sh <app_name> <app_version>" 1>&2
  exit 1
else
  APPLICATION="$1"
fi

if [[ -z "$2" ]]; then
  echo "Usage: build.sh <app_name> <app_version>" 1>&2
  exit 1
else
  APPLICATION_VERSION="$2"
fi

if [[ -z "$YC_TOKEN" ]]; then
  echo "Add YC_TOKEN to context"
  exit 1
fi
if [[ -z "$YT_TOKEN" ]]; then
  echo "Add YT_TOKEN to context"
  exit 1
fi

COMMIT_REVISION="${COMMIT_REVISION:-${RANDOM:-}}"

cd "$(dirname "$0")" || exit 2

YC_SOURCE_FOLDER_ID=b1grpl1006mpj1jtifi1
case $APPLICATION in
devices) ;;
migrator) ;;
mqtt) ;;
events) ;;
tank) ;;
subscriptions) ;;
*)
  echo "Unknown application $APPLICATION" 1>&2
  exit 1
  ;;
esac

cd ${APPLICATION} || exit 2

APPLICATION_VERSION=${APPLICATION_VERSION} \
  YC_FOLDER_ID=b1gq57e4mu8e8nhimac0 \
  YC_SUBNET_ID=b0c7crr1buiddqjmuhn7 \
  YC_SOURCE_FOLDER_ID=${YC_SOURCE_FOLDER_ID} \
  YC_ENDPOINT=api.cloud.yandex.net:443 \
  COMMIT_REVISION=${COMMIT_REVISION} \
  packer build --on-error=ask main.json
