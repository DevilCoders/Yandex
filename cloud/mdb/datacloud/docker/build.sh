#!/bin/bash

set -e

YC_PROFILE_NAME=$1


if [[ -d $2 ]]
then
  export PROJECT=$2
else
  echo "directory '$2' does not exist"
  exit 1
fi

if [[ $3 == "yc-compute-preprod" ]]; then
  export YANDEX_REGISTRY=cr.cloud-preprod.yandex.net
  export YANDEX_REPOSITORY=crtj2g6v2ipboa4hd0bp
elif [[ $3 == "yc-internal-mdb-dev" ]]; then
  export YANDEX_REGISTRY=cr.yandex
  export YANDEX_REPOSITORY=crpjrtim4vglpam52c0l
else
  echo "unknown environment $3"
  exit 1
fi

cd $PROJECT
make $4 YC_PROFILE_NAME=$YC_PROFILE_NAME YANDEX_REGISTRY=$YANDEX_REGISTRY YANDEX_REPOSITORY=$YANDEX_REPOSITORY
