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

YC_SOURCE_FOLDER_ID=aoe824hvnc67es4f8kqj
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
  YC_FOLDER_ID=aoe888pej579nq07j2nb \
  YC_SUBNET_ID=fo27jfhs8sfn4u51ak2s \
  YC_SOURCE_FOLDER_ID=${YC_SOURCE_FOLDER_ID} \
  YC_ENDPOINT=api.cloud-preprod.yandex.net:443 \
  COMMIT_REVISION=${COMMIT_REVISION} \
  packer build --on-error=ask main.json
